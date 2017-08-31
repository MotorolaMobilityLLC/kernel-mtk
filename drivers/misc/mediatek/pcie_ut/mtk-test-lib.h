#ifndef _MTK_TEST_LIB_
#define _MTK_TEST_LIB_

/********************************************************
 *	pcie
 *		pcie unit tests
 *	args:
*********************************************************/
extern int t_pcie_register_pci(int argc, char **argv);
extern int t_pcie_unregister_pci(int argc, char **argv);
extern int t_pcie_mmio(int argc, char **argv);
extern int t_pcie_setup_int(int argc, char **argv);
extern int t_pcie_aspm(int argc, char **argv);
extern int t_pcie_retrain(int argc, char **argv);
extern int t_pcie_aspm_retrain(int argc, char **argv);
extern int t_pcie_pcipm(int argc, char **argv);
extern int t_pcie_l1pm(int argc, char **argv);
extern int t_pcie_l1ss(int argc, char **argv);
extern int t_pcie_reset(int argc, char **argv);
extern int t_pcie_change_speed(int argc, char **argv);
extern int t_pcie_speed_test(int argc, char **argv);
extern int t_pcie_configuration(int argc, char **argv);
extern int t_pcie_link_width(int argc, char **argv);
extern int t_pcie_reset_config(int argc, char **argv);
extern int t_pcie_memory(int argc, char **argv);
extern int t_pcie_loopback(int argc, char **argv);

extern int t_pcie_loopback_cr4(int argc, char **argv);
extern int t_pcie_set_own(int argc, char **argv);
extern int t_pcie_clear_own(int argc, char **argv);
extern int t_pcie_test(int argc, char **argv);

#endif
