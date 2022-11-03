#include <linux/random.h>
#include "inc/tcpci.h"
#include "inc/pd_policy_engine.h"
#include "inc/pd_dpm_core.h"
#include "pd_dpm_prv.h"
#include "inc/tcpci_core.h"
#include <crypto/hash.h>

#ifdef CONFIG_SUPPORT_MMI_ADAPTER

#define CMD_HEADER_REQUEST          0x01
#define CMD_HEADER_ACK              0x02

#define CMD_CHARGER_CAPACITY        0x01
#define CMD_CHARGER_AUTHENTICATION  0x02
#define CMD_CHARGER_TEMPERATURE     0x03
#define CMD_CHARGER_VERIFY          0x04

#define CMD(header,cmd)                 ((header << 8) + cmd)
#define MMI_UVDM_CMD(vid,header,cmd) ((vid << 16) + CMD(header,cmd))

#define CONFIG_MOTO_ADAPTER         0x4d4f544f
#define CONFIG_ADAPTER_68W          0x30363841

#define SHA256_NUM                  4
static uint32_t random_num[SHA256_NUM] = {0};
#define MMI_KEY                  "Motorola-Hyper-Proprietary-2021Gen1"

enum MMI_STATE {
    MMI_NONE = 0,
    MMI_REQUEST_CAP,
    MMI_ENCRYPT,
    MMI_REQUEST_TEMP,
    MMI_VERIFY,
    MMI_READY,
};

static const char * const mmi_state[] = {
    "None",
    "Request cap",
    "Encrypt",
    "Request temp",
    "Verify",
    "mmi adapter ready",
};

static int hmac_sha256(u8 *key, u8 ksize, char *plaintext, u8 psize, u8 *output)
{
	struct crypto_shash *tfm;
	struct shash_desc *shash;
	int ret;

	if (!ksize)
		return -EINVAL;

	tfm = crypto_alloc_shash("hmac(sha256)", 0, 0);
	if (IS_ERR(tfm)) {
		MMI_INFO("crypto_alloc_ahash failed: err %ld", PTR_ERR(tfm));
		return PTR_ERR(tfm);
	}

	ret = crypto_shash_setkey(tfm, key, ksize);
	if (ret) {
		MMI_INFO("crypto_ahash_setkey failed: err %d", ret);
		goto failed;
	}

	shash = kzalloc(sizeof(*shash) + crypto_shash_descsize(tfm),
			GFP_KERNEL);
	if (!shash) {
		ret = -ENOMEM;
		goto failed;
	}

	shash->tfm = tfm;

	ret = crypto_shash_digest(shash, plaintext, psize, output);

	kfree(shash);

failed:
	crypto_free_shash(tfm);
	return ret;
}

static int mmi_hmac_sha256(u8 *key, u8 ksize,uint32_t *plaintext,
                                                u8 psize,uint32_t *output)
{
    uint8_t *data_in;
    uint8_t *data_out;
    int i = 0,j = 0;

    data_in = kzalloc(sizeof(*data_in) * psize * 4,GFP_KERNEL);
    if (!data_in) {
        return -ENOMEM;
    }

    data_out = kzalloc(sizeof(*data_out) * psize * 4,GFP_KERNEL);
    if (!data_out) {
        kfree(data_in);
        return -ENOMEM;
    }

    for (i = 0;i < psize;i++) {
        for (j = 0; j < sizeof(*plaintext); j++) {
            data_in[(i * 4) + 3 - j] = (plaintext[i] >> (j * 8)) & 0xff;
        }
    }

    hmac_sha256(key,ksize,data_in,4*psize,data_out);

    for (i = 0;i < psize;i++) {
        output[i] = data_out[3 + (4 * i)] + (data_out[2 + (4 * i)] << 8) +
                (data_out[1 + (4 * i)] << 16) + (data_out[0 + (4 * i)] << 24);
    }

    kfree(data_in);
    kfree(data_out);
    return 0;
}

static bool mmi_uvdm_ack(struct pd_port *pd_port,
                                    struct svdm_svid_data *svid_data)
{
    uint16_t vid = svid_data->svid;
    uint32_t data = pd_port->uvdm_data[0];
    if ((data >> 16) != vid) {
        return false;
    }
    if (((data & 0xff) == pd_port->mmi_adapter_state) &&
        ((data >> 8) & 0xff) == CMD_HEADER_ACK) {
        return true;
    }
    return false;
}

static void mmi_adapter_set_state(struct pd_port *pd_port,uint8_t state) {
    MMI_INFO("%s -> %s\n",mmi_state[pd_port->mmi_adapter_state],
                             mmi_state[state]);
    pd_port->mmi_adapter_state = state;
}


static void mmi_adapter_cap_request(struct pd_port *pd_port,
                                    struct svdm_svid_data *svid_data)
{
    pd_port->uvdm_cnt = 1;
	pd_port->uvdm_wait_resp = true;
	pd_port->uvdm_data[0] = MMI_UVDM_CMD(svid_data->svid,
                        CMD_HEADER_REQUEST,CMD_CHARGER_CAPACITY);
    pd_put_tcp_vdm_event(pd_port,TCP_DPM_EVT_UVDM);
}

static void mmi_adapter_encrypt(struct pd_port *pd_port,
                                    struct svdm_svid_data *svid_data)
{
    uint8_t index = 0;
    pd_port->uvdm_cnt = 1 + SHA256_NUM;
    pd_port->uvdm_wait_resp = true;

    pd_port->uvdm_data[0] = MMI_UVDM_CMD(svid_data->svid,
                        CMD_HEADER_REQUEST,CMD_CHARGER_AUTHENTICATION);
    for (index = 0;index < SHA256_NUM;index++) {
        get_random_bytes(&(random_num[index]),sizeof(*random_num));
        //random_num[index] = index + 1;
        pd_port->uvdm_data[index+1] = random_num[index];
    }
    pd_put_tcp_vdm_event(pd_port, TCP_DPM_EVT_UVDM);
}

static void mmi_adapter_req_temp(struct pd_port *pd_port,
                                    struct svdm_svid_data *svid_data)
{
    pd_port->uvdm_cnt = 1;
	pd_port->uvdm_wait_resp = true;
	pd_port->uvdm_data[0] = MMI_UVDM_CMD(svid_data->svid,
                        CMD_HEADER_REQUEST,CMD_CHARGER_TEMPERATURE);
    pd_put_tcp_vdm_event(pd_port,TCP_DPM_EVT_UVDM);
}

static void mmi_adapter_verify(struct pd_port *pd_port,
                                    struct svdm_svid_data *svid_data)
{
    pd_port->uvdm_cnt = 1;
	pd_port->uvdm_wait_resp = true;
	pd_port->uvdm_data[0] = MMI_UVDM_CMD(svid_data->svid,
                        CMD_HEADER_ACK,CMD_CHARGER_VERIFY);
    pd_put_tcp_vdm_event(pd_port,TCP_DPM_EVT_UVDM);
}

bool mmi_dfp_notify_pe_startup(
	struct pd_port *pd_port, struct svdm_svid_data *svid_data)
{
    mmi_adapter_set_state(pd_port,MMI_NONE);
    return true;
}

bool mmi_dfp_u_notify_discover_svid(
	struct pd_port *pd_port, struct svdm_svid_data *svid_data, bool ack)
{
    MMI_INFO("discover\n");
    if (!ack) {
        return false;
    }
    mmi_adapter_set_state(pd_port,MMI_REQUEST_CAP);
    return true;
}

bool mmi_dfp_notify_uvdm(struct pd_port *pd_port,
		struct svdm_svid_data *svid_data, bool ack)
{
    uint32_t out[SHA256_NUM] = {0};
    char key[] = MMI_KEY;
    int i = 0;
    switch(pd_port->mmi_adapter_state) {
    case MMI_REQUEST_CAP:
        if (mmi_uvdm_ack(pd_port,svid_data) &&
                    pd_port->uvdm_data[1] == CONFIG_MOTO_ADAPTER &&
                    pd_port->uvdm_data[2] == CONFIG_ADAPTER_68W) {
#ifdef CONFIG_SUPPORT_MMI_ADAPTER_GET_TEMP
            mmi_adapter_set_state(pd_port,MMI_REQUEST_TEMP);
#else
            mmi_adapter_set_state(pd_port,MMI_ENCRYPT);
#endif /* CONFIG_SUPPORT_MMI_ADAPTER_GET_TEMP */
            return true;
        }
        break;
    case MMI_REQUEST_TEMP:
        if (mmi_uvdm_ack(pd_port,svid_data)) {
            MMI_INFO("now adapter temp = %d\n",pd_port->uvdm_data[1]);
            mmi_adapter_set_state(pd_port,MMI_ENCRYPT);
            return true;
        }
        break;
    case MMI_ENCRYPT:
        if (mmi_uvdm_ack(pd_port,svid_data)) {
            /**
             * todo hmac-sha256 check
             * src data : pd_port->uvdm_data[1-4]
             * random   : random_num
             */
            mmi_hmac_sha256(key,sizeof(key),random_num,SHA256_NUM,out);
            for (i = 0;i < SHA256_NUM;i++) {
                if (pd_port->uvdm_data[i+1] != out[i])
                    goto out;
            }
            mmi_adapter_set_state(pd_port,MMI_VERIFY);
            return true;
        }
        break;
    case MMI_VERIFY:
        if (mmi_uvdm_ack(pd_port,svid_data) &&
                       pd_port->uvdm_data[1] == 0x01) {
            mmi_adapter_set_state(pd_port,MMI_READY);
            dpm_reaction_set(pd_port, DPM_REACTION_GET_SOURCE_CAP);
            return true;
        }
        break;
    default:
        break;
    }
out:
    mmi_adapter_set_state(pd_port,MMI_NONE);
    return true;
}

int mmi_notify_pe_ready(struct pd_port *pd_port,
		struct svdm_svid_data *svid_data)
{
    switch(pd_port->mmi_adapter_state) {
    case MMI_REQUEST_CAP:
        mmi_adapter_cap_request(pd_port,svid_data);
        break;
    case MMI_REQUEST_TEMP:
        mmi_adapter_req_temp(pd_port,svid_data);
        break;
    case MMI_ENCRYPT:
        mmi_adapter_encrypt(pd_port,svid_data);
        break;
    case MMI_VERIFY:
        mmi_adapter_verify(pd_port,svid_data);
        break;
    case MMI_READY:
        pd_update_vdm_verify_state(pd_port, true);
        break;
    default:
        break;
    }
    return 0;
}

#endif /* CONFIG_SUPPORT_MMI_ADAPTER */