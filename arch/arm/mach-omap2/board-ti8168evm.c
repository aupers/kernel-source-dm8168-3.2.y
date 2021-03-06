/*
 * Code for TI8168/TI8148 EVM.
 *
 * Copyright (C) 2010 Texas Instruments, Inc. - http://www.ti.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/kernel.h>
#include <linux/init.h>

#ifdef CONFIG_REGULATOR_GPIO
#include <linux/gpio.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/gpio-regulator.h>
#endif

#include <linux/spi/spi.h>
#include <linux/spi/flash.h>
#include <linux/mtd/physmap.h>
#include <linux/i2c/at24.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/ti81xxfb.h>
#include <linux/ti81xx.h>
#include <linux/vps_capture.h>
#include <sound/tlv320aic3x.h>

#include <mach/hardware.h>
#include <mach/board-ti816x.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>

#include <plat/irqs.h>
#include <plat/board.h>
#include <plat/mmc.h>
#include <plat/usb.h>
#include <plat/asp.h>
#include <plat/hdmi_lib.h>

#include "common.h"
#include "mux.h"
#include "hsmmc.h"

static struct omap_board_config_kernel ti81xx_evm_config[] __initdata = {
};

#ifdef CONFIG_OMAP_MUX
static struct omap_board_mux board_mux[] __initdata = {

	/* PIN mux for non-muxed NOR */
	TI816X_MUX(TIM7_OUT, OMAP_MUX_MODE1),	/* gpmc_a12 */
	TI816X_MUX(UART1_CTSN, OMAP_MUX_MODE1),	/* gpmc_a13 */
	TI816X_MUX(UART1_RTSN, OMAP_MUX_MODE1),	/* gpmc_a14 */
	TI816X_MUX(UART2_RTSN, OMAP_MUX_MODE1),	/* gpmc_a15 */
	/* REVISIT: why 2 lines configured as gpmc_a15 */
	TI816X_MUX(SC1_RST, OMAP_MUX_MODE1),	/* gpmc_a15 */
	TI816X_MUX(UART2_CTSN, OMAP_MUX_MODE1),	/* gpmc_a16 */
	TI816X_MUX(UART0_RIN, OMAP_MUX_MODE1),	/* gpmc_a17 */
	TI816X_MUX(UART0_DCDN, OMAP_MUX_MODE1),	/* gpmc_a18 */
	TI816X_MUX(UART0_DSRN, OMAP_MUX_MODE1),	/* gpmc_a19 */
	TI816X_MUX(UART0_DTRN, OMAP_MUX_MODE1),	/* gpmc_a20 */
	TI816X_MUX(SPI_SCS3, OMAP_MUX_MODE1),	/* gpmc_a21 */
	TI816X_MUX(SPI_SCS2, OMAP_MUX_MODE1),	/* gpmc_a22 */
	TI816X_MUX(GP0_IO6, OMAP_MUX_MODE2),	/* gpmc_a23 */
	TI816X_MUX(TIM6_OUT, OMAP_MUX_MODE1),	/* gpmc-a24 */
	TI816X_MUX(SC0_DATA, OMAP_MUX_MODE1),	/* gpmc_a25 */
	/* for controlling high address */
	TI816X_MUX(GPMC_A27, OMAP_MUX_MODE1),	/* gpio-20 */
	{ .reg_offset = OMAP_MUX_TERMINATOR },
};
#else
#define board_mux	NULL
#endif

static struct omap2_hsmmc_info mmc[] = {
	{
		.mmc		= 1,
		.caps		= MMC_CAP_4_BIT_DATA,
		.gpio_cd	= -EINVAL,/* Dedicated pins for CD and WP */
		.gpio_wp	= -EINVAL,
		.ocr_mask	= MMC_VDD_33_34,
	},
	{}	/* Terminator */
};

static struct omap_musb_board_data musb_board_data = {
	.interface_type	= MUSB_INTERFACE_ULPI,
	.mode           = MUSB_HOST,
	.power		= 500,
	.instances	= 1,
};

#ifdef CONFIG_REGULATOR_GPIO
static struct regulator_consumer_supply ti816x_gpio_dcdc_supply[] = {
	{
		.supply = "vdd_avs",
	},
};

static struct regulator_init_data ti816x_gpio_vr_init_data = {
	.constraints = {
		.min_uV		= 800000,
		.max_uV		= 1025000,
		.valid_ops_mask	= (REGULATOR_CHANGE_VOLTAGE |
						   REGULATOR_CHANGE_STATUS),
	},
	.consumer_supplies	= ti816x_gpio_dcdc_supply,
	.num_consumer_supplies	= ARRAY_SIZE(ti816x_gpio_dcdc_supply),
};

/* Supported voltage values for regulators */
static struct gpio_regulator_state ti816x_states_table[] = {
	{ .value = 800000, .gpios = 0x0 },
	{ .value = 815000, .gpios = 0x8 },
	{ .value = 830000, .gpios = 0x4 },
	{ .value = 845000, .gpios = 0xC },
	{ .value = 860000, .gpios = 0x2 },
	{ .value = 875000, .gpios = 0xA },
	{ .value = 890000, .gpios = 0x6 },
	{ .value = 905000, .gpios = 0xE },
	{ .value = 920000, .gpios = 0x1 },
	{ .value = 935000, .gpios = 0x9 },
	{ .value = 950000, .gpios = 0x5 },
	{ .value = 965000, .gpios = 0xD },
	{ .value = 980000, .gpios = 0x3 },
	{ .value = 995000, .gpios = 0xB },
	{ .value = 1010000, .gpios = 0x7},
	{ .value = 1025000, .gpios = 0xF},
};

/* Macro for GPIO voltage regulator */
#define VR_GPIO_INSTANCE	0

static struct gpio ti816x_vcore_gpios[] = {
	{ (VR_GPIO_INSTANCE * 32) + 0, GPIOF_OUT_INIT_HIGH /* GPIOF_IN */, "vgpio 0"},
	{ (VR_GPIO_INSTANCE * 32) + 1, GPIOF_OUT_INIT_HIGH /* GPIOF_IN */, "vgpio 1"},
	{ (VR_GPIO_INSTANCE * 32) + 2, GPIOF_OUT_INIT_HIGH /* GPIOF_IN */, "vgpio 2"},
	{ (VR_GPIO_INSTANCE * 32) + 3, GPIOF_OUT_INIT_HIGH /* GPIOF_IN */, "vgpio 3"},
};

/* GPIO regulator platform data */
static struct gpio_regulator_config ti816x_gpio_vr_config = {
	.supply_name	= "VFB",
	.enable_gpio	= -EINVAL,
	.enable_high	= 1,
	.enabled_at_boot = 1,
	.startup_delay	= 0,
	.gpios			= ti816x_vcore_gpios,
	.nr_gpios		= ARRAY_SIZE(ti816x_vcore_gpios),
	.states			= ti816x_states_table,
	.nr_states		= ARRAY_SIZE(ti816x_states_table),
	.type			= REGULATOR_VOLTAGE,
	.init_data		= &ti816x_gpio_vr_init_data,
};

/* VCORE for SR regulator init */
static struct platform_device ti816x_gpio_vr_device = {
	.name	= "gpio-regulator",
	.id		= -1,
	.dev = {
		.platform_data = &ti816x_gpio_vr_config,
	},
};

static void __init ti816x_gpio_vr_init(void)
{
	if (platform_device_register(&ti816x_gpio_vr_device))
		pr_err("failed to register ti816x_gpio_vr device\n");
	else
		pr_info("registered ti816x_gpio_vr device\n");
}
#else
static inline void ti816x_gpio_vr_init(void) {}
#endif

struct mtd_partition ti816x_spi_partitions[] = {
	/* All the partition sizes are listed in terms of NAND block size */
	{
		.name		= "U-Boot",
		.offset		= 0,	/* Offset = 0x0 */
		.size		= 64 * SZ_4K,
		.mask_flags	= MTD_WRITEABLE,	/* force read-only */
	},
	{
		.name		= "U-Boot Env",
		.offset		= MTDPART_OFS_APPEND,	/* Offset = 0x40000 */
		.size		= 2 * SZ_4K,
	},
	{
		.name		= "Kernel",
		.offset		= MTDPART_OFS_APPEND,	/* Offset = 0x42000 */
		.size		= 640 * SZ_4K,
	},
	{
		.name		= "File System",
		.offset		= MTDPART_OFS_APPEND,	/* Offset = 0x2C2000 */
		.size		= MTDPART_SIZ_FULL,	/* size = 1.24 MiB */
	}
};

const struct flash_platform_data ti816x_spi_flash = {
	.name		= "w25q32",
	.parts		= ti816x_spi_partitions,
	.nr_parts	= ARRAY_SIZE(ti816x_spi_partitions),
};

struct spi_board_info __initdata ti816x_spi_slave_info[] = {
	{
		.modalias		= "w25q32",
		.platform_data	= &ti816x_spi_flash,
		.irq			= -1,
		.max_speed_hz	= 48000000, /* max spi freq */
		.bus_num		= 1,
		.chip_select	= 0,
	},
};

static void __init ti816x_spi_init(void)
{
	spi_register_board_info(ti816x_spi_slave_info,
							ARRAY_SIZE(ti816x_spi_slave_info));
}

static struct i2c_board_info __initdata ti816x_i2c_boardinfo0[] = {
	{
		I2C_BOARD_INFO("24c256", 0x50),
		.flags = I2C_M_TEN,
	},
	{
		I2C_BOARD_INFO("tlv320aic3x", 0x18),
	},
	{
		I2C_BOARD_INFO("pcf8575", 0x20),
	},
};

static struct i2c_board_info __initdata ti816x_i2c_boardinfo1[] = {
	{
		I2C_BOARD_INFO("pcf8575_ths7360", 0x20), /* this IO Expander interacts with THS7375 and THS7360 video amplifiers */
	},
	{
		I2C_BOARD_INFO("pcf8575_ths7368", 0x21), /* this IO Expander interacts with THS7368 video amplifier */
	},
};

static int __init ti816x_evm_i2c_init(void)
{
	omap_register_i2c_bus(1, 10, ti816x_i2c_boardinfo0,
		ARRAY_SIZE(ti816x_i2c_boardinfo0));
	omap_register_i2c_bus(2, 10, ti816x_i2c_boardinfo1,
		ARRAY_SIZE(ti816x_i2c_boardinfo1));
	return 0;
}

static u8 ti8168_iis_serializer_direction[] = {
	TX_MODE,	RX_MODE,	INACTIVE_MODE,	INACTIVE_MODE,
	INACTIVE_MODE,	INACTIVE_MODE,	INACTIVE_MODE,	INACTIVE_MODE,
	INACTIVE_MODE,	INACTIVE_MODE,	INACTIVE_MODE,	INACTIVE_MODE,
	INACTIVE_MODE,	INACTIVE_MODE,	INACTIVE_MODE,	INACTIVE_MODE,
};

static struct snd_platform_data ti8168_evm_snd_data = {
	.tx_dma_offset	= 0x46800000,
	.rx_dma_offset	= 0x46800000,
	.op_mode	= DAVINCI_MCASP_IIS_MODE,
	.num_serializer = ARRAY_SIZE(ti8168_iis_serializer_direction),
	.tdm_slots	= 2,
	.serial_dir	= ti8168_iis_serializer_direction,
	.asp_chan_q	= EVENTQ_2,
	.version	= MCASP_VERSION_2,
	.txnumevt	= 1,
	.rxnumevt	= 1,
};

#ifdef CONFIG_SND_SOC_TI81XX_HDMI
static struct snd_hdmi_platform_data ti8168_snd_hdmi_pdata = {
	.dma_addr = TI81xx_HDMI_WP + HDMI_WP_AUDIO_DATA,
	.channel = 53,
	.data_type = 4,
	.acnt = 4,
	.fifo_level = 0x20,
};

static struct platform_device ti8168_hdmi_audio_device = {
	.name	= "hdmi-dai",
	.id	= -1,
        .dev = {
		.platform_data = &ti8168_snd_hdmi_pdata,
        }
};

static struct platform_device ti8168_hdmi_codec_device = {
	.name	= "hdmi-dummy-codec",
	.id	= -1,
};

static struct platform_device *ti8168_devices[] __initdata = {
	&ti8168_hdmi_audio_device,
	&ti8168_hdmi_codec_device,
};

/*
 * TI8168 PG2.0 support Auto CTS which needs MCLK .
 * McBSP clk is used as MCLK in PG2.0 which have 4 parent clks
 * sysclk20, sysclk21, sysclk21 and CLKS(external)
 * Currently we are using sysclk22 as the parent for McBSP clk
 * ToDo:
*/
void __init ti8168_hdmi_mclk_init(void)
{
	int ret = 0;
	struct clk *parent, *child;

	/* modify the clk name from list to use different clk source */
	parent = clk_get(NULL, "sysclk22_ck");
	if (IS_ERR(parent))
		pr_err("Unable to get [sysclk22_ck] clk\n");

	/* get HDMI dev clk*/
	child = clk_get(NULL, "hdmi_i2s_ck");
	if (IS_ERR(child))
		pr_err("Unable to get [hdmi_i2s_ck] clk\n");

	ret = clk_set_parent(child, parent);
	if (ret < 0)
		pr_err("Unable to set parent [hdmi_ck] clk\n");

	pr_debug("HDMI: Audio MCLK setup complete!\n");

}
#endif

#if defined(CONFIG_TI81XX_VPSS) || defined(CONFIG_TI81XX_VPSS_MODULE)

static u64 ti81xx_dma_mask = ~(u32)0;

static struct platform_device vpss_device = {
	.name = "vpss",
	.id = -1,
	.dev = {
		.platform_data = NULL,
	},
};

#if defined (CONFIG_TI816X_THS7360)
int pcf8575_ths7360_hd_enable(enum sf_channel_filter_ctrl ctrl);
int pcf8575_ths7360_sd_enable(enum sd_channel_filter_ctrl ctrl);
#else
int pcf8575_ths7360_hd_enable(enum sf_channel_filter_ctrl ctrl)
{
	return 0;
}
int pcf8575_ths7360_sd_enable(enum sd_channel_filter_ctrl ctrl)
{
	return 0;
}
#endif

static struct vps_platform_data vps_pdata = {
	.cpu = CPU_DM816X,
	.hd_channel_ctrl = pcf8575_ths7360_hd_enable,
	.sd_channel_ctrl = pcf8575_ths7360_sd_enable,
	.numvencs = 4,
	.vencmask = (1 << VPS_DC_MAX_VENC) - 1,

	/*set up the grpx connections*/
	.numgrpx = VPS_DISP_GRPX_MAX_INST,
	.gdata = {
		/*grpx0 out to hdmi*/
		{ .snode = VPS_DC_GRPX0_INPUT_PATH, .numends = 1, .enode[0] = VPS_DC_HDMI_BLEND },
		/*grpx1 out to HDCOMP(DM816X)*/
		{ .snode = VPS_DC_GRPX1_INPUT_PATH, .numends = 1, .enode[0] = VPS_DC_HDCOMP_BLEND },
		/*grpx2 out to SD*/
		{ .snode = VPS_DC_GRPX2_INPUT_PATH, .numends = 1, .enode[0] = VPS_DC_SDVENC_BLEND }
	},

	.numvideo = 3,
	/*set up the v4l2 video display connections*/
	.vdata = {
		{
			/*/dev/video1 -> HDMI*/
			.idx = 0,
			.numedges = 2,
			.snodes[0] = VPS_DC_VCOMP_MUX,
			.snodes_inputid[0] = VPS_DC_BP0_INPUT_PATH,
			.snodes[1] = VPS_DC_VCOMP,
			.snodes_inputid[1] = VPS_DC_VCOMP_MUX,
			.numoutput = 1,
			.enodes[0] = VPS_DC_HDMI_BLEND,
			.enodes_inputid[0] = VPS_DC_CIG_NON_CONSTRAINED_OUTPUT
		}, {
			/*/dev/video2 -> HDCOMP or DVO2*/
			.idx = 0,
			.numedges = 2,
			.snodes[0] = VPS_DC_VCOMP_MUX,
			.snodes_inputid[0] = VPS_DC_BP0_INPUT_PATH,
			.snodes[1] = VPS_DC_VCOMP,
			.snodes_inputid[1] = VPS_DC_VCOMP_MUX,
			.numoutput = 1,
			.enodes[0] = VPS_DC_HDMI_BLEND,
			.enodes_inputid[0] = VPS_DC_CIG_NON_CONSTRAINED_OUTPUT
		}, {
			/*/dev/video3 is SD only*/
			.idx = 2,
			.numedges = 1,
			.snodes_inputid[0] = VPS_DC_SEC1_INPUT_PATH,
			.snodes[0] = VPS_DC_SDVENC_MUX,
			.numoutput = 1,
			.enodes[0] = VPS_DC_SDVENC_BLEND,
			.enodes_inputid[0] = VPS_DC_SDVENC_MUX
		}
	},
};

static int __init ti81xx_vpss_init(void)
{
	int r;
	vpss_device.dev.platform_data = &vps_pdata;

	r = platform_device_register(&vpss_device);
	if (r)
		printk(KERN_ERR "unable to register ti81xx_vpss device\n");
	else
		printk(KERN_INFO "registered ti81xx_vpss device\n");
	return r;
}
#endif

#if defined(CONFIG_TI81XX_HDMI_MODULE) || defined(CONFIG_TI81XX_HDMI)

static struct platform_device ti81xx_hdmi_plat_device = {
	.name = "TI81XX_HDMI",
	.id = -1,
	.num_resources = 0,
	.dev = {
		.platform_data = NULL,
	}
};

static int __init ti81xx_hdmi_init(void)
{
	int r;
	r = platform_device_register(&ti81xx_hdmi_plat_device);
	if (r)
		printk(KERN_ERR "Unable to register ti81xx onchip-HDMI device\n");
	else
		printk(KERN_INFO "registered ti81xx on-chip HDMI device\n");
	return r;
}
#else
static int __init ti81xx_hdmi_init(void)
{
	return 0;
}
#endif

#if defined(CONFIG_FB_TI81XX_MODULE) || defined(CONFIG_FB_TI81XX)
static struct ti81xxfb_platform_data ti81xxfb_config;

static struct platform_device ti81xx_fb_device = {
	.name		= "ti81xxfb",
	.id		= -1,
	.dev = {
		.dma_mask		= &ti81xx_dma_mask,
		.coherent_dma_mask	= ~(u32)0,
		.platform_data		= &ti81xxfb_config,
	},
	.num_resources = 0,
};


void ti81xxfb_set_platform_data(struct ti81xxfb_platform_data *data)
{
	ti81xxfb_config = *data;
}

static int __init ti81xx_fb_init(void)
{
	int r;
	r = platform_device_register(&ti81xx_fb_device);
	if (r)
		printk(KERN_ERR "unable to register ti81xx_fb device\n");
	else
		printk(KERN_INFO "registered ti81xx_fb device\n");
	return r;

}
#else
static int __init ti81xx_fb_init(void)
{
	return 0;
}
void ti81xxfb_set_platform_data(struct ti81xxfb_platform_data *data)
{
}
#endif

#if defined(CONFIG_VIDEO_TI81XX_VIDOUT_MODULE) || defined(CONFIG_VIDEO_TI81XX_VIDOUT)
static struct resource ti81xx_vidout_resource[VPS_DISPLAY_INST_MAX] = {
};

static struct platform_device ti81xx_vidout_device = {
	.name		= "ti81xx_vidout",
	.num_resources  = ARRAY_SIZE(ti81xx_vidout_resource),
	.resource       = &ti81xx_vidout_resource[0],
	.id             = -1,
};

static int __init ti81xx_vout_init(void)
{
	int r;

	r = platform_device_register(&ti81xx_vidout_device);
	if (r)
		printk(KERN_ERR "Unable to register ti81xx_vidout device\n");
	else
		printk(KERN_INFO "registered ti81xx_vidout device\n");
	return r;
}
#else
static int __init ti81xx_vout_init(void)
{
	return 0;
}
#endif

#if defined(CONFIG_VIDEO_TI81XX_VIDIN_MODULE) || defined(CONFIG_VIDEO_TI81XX_VIDIN)

#define HDVPSS_CAPTURE_INST0_BASE	0x48105500
#define HDVPSS_CAPTURE_INST0_SIZE	1024u

#define HDVPSS_CAPTURE_INST2_BASE	0x48105A00
#define HDVPSS_CAPTURE_INST2_SIZE	1024u

#if defined (CONFIG_TI816X_THS7368)
int pcf8575_ths7368_select_video_decoder(int vid_decoder_id);
int pcf8575_ths7368_set_tvp7002_filter(enum fvid2_standard standard);
#else
int pcf8575_ths7368_select_video_decoder(int vid_decoder_id)
{
	return 0;
}
int pcf8575_ths7368_set_tvp7002_filter(enum fvid2_standard standard)
{
	return 0;
}
#endif

u8 ti81xx_card_name[] = "TI81xx_catalogue";
struct ti81xxvin_interface tvp7002_pdata = {
	.clk_polarity = 0,
	.hs_polarity = 0,
	.vs_polarity = 1,
	.fid_polarity = 0,
	.sog_polarity = 0,

};
static struct ti81xxvin_subdev_info hdvpss_capture_sdev_info[] = {
	{
		.name	= TVP7002_INST0,
		.board_info = {
			/* TODO Find the correct address
				of the TVP7002 connected */
			I2C_BOARD_INFO("tvp7002", 0x5d),
			.platform_data = &tvp7002_pdata,
		},
		.vip_port_cfg = {
			.ctrlChanSel = VPS_VIP_CTRL_CHAN_SEL_15_8,
			.ancChSel8b = VPS_VIP_ANC_CH_SEL_DONT_CARE,
			.pixClkEdgePol = VPS_VIP_PIX_CLK_EDGE_POL_RISING,
			.invertFidPol = 0,
			.embConfig = {
				.errCorrEnable = 1,
				.srcNumPos = VPS_VIP_SRC_NUM_POS_DONT_CARE,
				.isMaxChan3Bits = 0,
			},
			.disConfig = {
				.fidSkewPostCnt = 0,
				.fidSkewPreCnt = 0,
				.lineCaptureStyle =
					VPS_VIP_LINE_CAPTURE_STYLE_DONT_CARE,
				.fidDetectMode =
					VPS_VIP_FID_DETECT_MODE_DONT_CARE,
				.actvidPol = VPS_VIP_POLARITY_DONT_CARE,
				.vsyncPol =  VPS_VIP_POLARITY_DONT_CARE,
				.hsyncPol = VPS_VIP_POLARITY_DONT_CARE,
			}
		},
		.video_capture_mode =
		   VPS_CAPT_VIDEO_CAPTURE_MODE_SINGLE_CH_NON_MUX_EMBEDDED_SYNC,
		.video_if_mode = VPS_CAPT_VIDEO_IF_MODE_16BIT,
		.input_data_format = FVID2_DF_YUV422P,
		.ti81xxvin_select_decoder =	pcf8575_ths7368_select_video_decoder,
		.ti81xxvin_set_mode = pcf8575_ths7368_set_tvp7002_filter,
		.decoder_id = 0,
	},
};

static const struct v4l2_dv_preset hdvpss_inst0_inp0_presets[] = {
	{
		.preset = V4L2_DV_720P60,
	},
	{
		.preset = V4L2_DV_1080I60,
	},
	{
		.preset = V4L2_DV_1080P60,
	},
	{
		.preset = V4L2_DV_1080P30,
	},
};

static const struct v4l2_dv_preset hdvpss_inst2_inp0_presets[] = {
	{
		.preset = V4L2_DV_720P60,
	},
	{
		.preset = V4L2_DV_1080I60,
	},
	{
		.preset = V4L2_DV_1080P60,
	},
	{
		.preset = V4L2_DV_1080P30,
	},
};

static const struct ti81xxvin_input hdvpss_inst0_inputs[] = {
	{
		.input = {
			.index		= 0,
			.name		= "Component",
			.type		= V4L2_INPUT_TYPE_CAMERA,
			.std		= V4L2_STD_UNKNOWN,
			.capabilities	= V4L2_OUT_CAP_PRESETS,
		},
		.subdev_name	= TVP7002_INST0,
		.dv_presets	= hdvpss_inst0_inp0_presets,
		.num_dv_presets	= ARRAY_SIZE(hdvpss_inst0_inp0_presets),
	},
};

static const struct ti81xxvin_input hdvpss_inst1_inputs[] = {
	{
		.input = {
			.index		= 0,
			.name		= "Component",
			.type		= V4L2_INPUT_TYPE_CAMERA,
			.std		= V4L2_STD_UNKNOWN,
			.capabilities	= V4L2_OUT_CAP_PRESETS,
		},
		.subdev_name	= TVP7002_INST1,
		.dv_presets	= hdvpss_inst2_inp0_presets,
		.num_dv_presets	= ARRAY_SIZE(hdvpss_inst2_inp0_presets),
	},
};

static struct ti81xxvin_config ti81xx_hsvpss_capture_cfg = {
	.subdev_info = hdvpss_capture_sdev_info,
	.subdev_count = ARRAY_SIZE(hdvpss_capture_sdev_info),
	.card_name = ti81xx_card_name,
	.inst_config[0] = {
		.inputs = hdvpss_inst0_inputs,
		.input_count = ARRAY_SIZE(hdvpss_inst0_inputs),
	},
	.inst_config[1] = {
		.inputs = hdvpss_inst0_inputs,
		.input_count = 0,
	},
	.inst_config[2] = {
		.inputs = hdvpss_inst1_inputs,
		.input_count = ARRAY_SIZE(hdvpss_inst1_inputs),
	},
	.inst_config[3] = {
		.inputs = hdvpss_inst1_inputs,
		.input_count = 0,
	},
};

static struct resource ti81xx_hdvpss_capture_resource[] = {
	[0] = {
		.start = HDVPSS_CAPTURE_INST0_BASE,
		.end   = (HDVPSS_CAPTURE_INST0_BASE +
				HDVPSS_CAPTURE_INST0_SIZE - 1),
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = HDVPSS_CAPTURE_INST2_BASE,
		.end   = (HDVPSS_CAPTURE_INST2_BASE +
				HDVPSS_CAPTURE_INST2_SIZE - 1),
		.flags = IORESOURCE_MEM,
	},
};

static struct platform_device hdvpss_capture_dev = {
	.name		= "ti81xxvin",
	.id		= -1,
	.dev		= {
			.dma_mask		= &ti81xx_dma_mask,
			.coherent_dma_mask	= ~(u32)0,
	},
	.num_resources = 2,
	.resource = ti81xx_hdvpss_capture_resource,
};

static int __init ti81xx_vin_init(void)
{
	int r;
	hdvpss_capture_dev.dev.platform_data = &ti81xx_hsvpss_capture_cfg;
	r = platform_device_register(&hdvpss_capture_dev);
	if (r)
		printk(KERN_ERR "unable to register ti81xx_vin device\n");
	else
		printk(KERN_INFO "registered ti81xx_vin device\n");
	return r;

}
#else
static int __init ti81xx_vin_init(void)
{
	return 0;
}
#endif

static void __init ti81xx_evm_init(void)
{
	ti81xx_mux_init(board_mux);
	omap_serial_init();
	omap_sdrc_init(NULL, NULL);
	omap_board_config = ti81xx_evm_config;
	omap_board_config_size = ARRAY_SIZE(ti81xx_evm_config);
	omap2_hsmmc_init(mmc);
	usb_musb_init(&musb_board_data);
	if (cpu_is_ti816x()) {
#ifdef CONFIG_REGULATOR_GPIO
		ti816x_gpio_vr_init();
		regulator_has_full_constraints();
		regulator_use_dummy_regulator();
#endif
		ti816x_spi_init();
		ti816x_evm_i2c_init();
		ti81xx_register_mcasp(0, &ti8168_evm_snd_data);
	}
#ifdef CONFIG_SND_SOC_TI81XX_HDMI
	ti8168_hdmi_mclk_init();
	platform_add_devices(ti8168_devices, ARRAY_SIZE(ti8168_devices));
#endif
	ti81xx_vpss_init();
	ti81xx_hdmi_init();
	ti81xx_fb_init();
	ti81xx_vout_init();
	ti81xx_vin_init();
}

MACHINE_START(TI8168EVM, "ti8168evm")
	/* Maintainer: Texas Instruments */
	.atag_offset	= 0x100,
	.map_io		= ti81xx_map_io,
	.reserve	= ti81xx_reserve,
	.init_early	= ti81xx_init_early,
	.init_irq	= ti81xx_init_irq,
	.timer		= &omap3_timer,
	.init_machine	= ti81xx_evm_init,
MACHINE_END

MACHINE_START(TI8148EVM, "ti8148evm")
	/* Maintainer: Texas Instruments */
	.atag_offset	= 0x100,
	.map_io		= ti81xx_map_io,
	.init_early	= ti81xx_init_early,
	.init_irq	= ti81xx_init_irq,
	.timer		= &omap3_timer,
	.init_machine	= ti81xx_evm_init,
MACHINE_END
