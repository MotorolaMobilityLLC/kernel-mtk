#ifndef CTS_TEST_H
#define CTS_TEST_H

struct cts_device;

extern int cts_short_test(struct cts_device *cts_dev, u16 threshold);
extern int cts_open_test(struct cts_device *cts_dev, u16 threshold);
extern int cts_reset_test(struct cts_device *cts_dev);
extern int cts_compensate_cap_test(struct cts_device *cts_dev, u8 min_thres,
				   u8 max_thres);
extern int cts_rawdata_test(struct cts_device *cts_dev, u16 min_thres,
			    u16 max_thres);
extern int cts_noise_test(struct cts_device *cts_dev, u32 frames, u16 max);
extern int cts_test_int_pin(struct cts_device *cts_dev);

#endif /* CTS_TEST_H */
