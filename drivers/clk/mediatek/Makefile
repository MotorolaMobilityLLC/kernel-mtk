ifneq ($(CONFIG_ARCH_MT6735),y)
	obj-y += clk-mtk.o clk-pll.o clk-gate.o
else
	obj-y += clk-mtk-v1.o clk-pll-v1.o clk-gate-v1.o
	ifneq ($(CONFIG_MTK_CLKMGR),y)
		obj-y += clk-mt6735.o clk-mt6735-pll.o clk-mt6735-pg.o
	endif
endif
