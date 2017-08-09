ifneq ($(CONFIG_ARCH_MT6735),y)
	obj-y += clk-mtk.o clk-pll.o clk-gate.o
	obj-$(CONFIG_RESET_CONTROLLER) += reset.o
	obj-$(CONFIG_ARCH_MT8135) += clk-mt8135.o
	obj-$(CONFIG_ARCH_MT8163) += clk-mt8163.o
	obj-$(CONFIG_ARCH_MT8173) += clk-mt8173.o
else
	obj-y += clk-mtk-v1.o clk-pll-v1.o clk-gate-v1.o
	ifneq ($(CONFIG_MTK_CLKMGR),y)
		obj-y += clk-mt6735.o clk-mt6735-pll.o clk-mt6735-pg.o
	endif
endif
