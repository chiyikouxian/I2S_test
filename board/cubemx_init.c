/*
 * CubeMX外设初始化
 * 从CubeMX生成的main.c提取
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "stm32h7rsxx_hal.h"

/* Error Handler */
void Error_Handler(void)
{
    rt_kprintf("HAL Error occurred!\n");
    while (1)
    {
        rt_thread_mdelay(1000);
    }
}

/* 全局外设句柄定义 */
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef handle_GPDMA1_Channel0;
DMA_NodeTypeDef Node_GPDMA1_Channel0 __attribute__((section("noncacheable_buffer")));
DMA_QListTypeDef List_GPDMA1_Channel0;

I2S_HandleTypeDef hi2s1;

/* 外设初始化函数（从CubeMX生成的main.c复制） */

void MX_GPDMA1_Init(void)
{
    /* Peripheral clock enable */
    __HAL_RCC_GPDMA1_CLK_ENABLE();

    /* GPDMA1 interrupt Init */
    HAL_NVIC_SetPriority(GPDMA1_Channel0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel0_IRQn);
}

void MX_ADC1_Init(void)
{
    ADC_MultiModeTypeDef multimode = {0};
    ADC_ChannelConfTypeDef sConfig = {0};

    /** Common config */
    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV16;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    hadc1.Init.LowPowerAutoWait = DISABLE;
    hadc1.Init.ContinuousConvMode = ENABLE;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.SamplingMode = ADC_SAMPLING_MODE_NORMAL;
    hadc1.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DMA_CIRCULAR;
    hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
    hadc1.Init.OversamplingMode = DISABLE;
    
    if (HAL_ADC_Init(&hadc1) != HAL_OK)
    {
        Error_Handler();
    }

    /** Configure the ADC multi-mode */
    multimode.Mode = ADC_MODE_INDEPENDENT;
    if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
    {
        Error_Handler();
    }

    /** Configure Regular Channel */
    sConfig.Channel = ADC_CHANNEL_5;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_92CYCLES_5;
    sConfig.SingleDiff = ADC_SINGLE_ENDED;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset = 0;
    sConfig.OffsetSign = ADC_OFFSET_SIGN_NEGATIVE;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }
}

void MX_I2S1_Init(void)
{
    hi2s1.Instance = SPI1;
    hi2s1.Init.Mode = I2S_MODE_MASTER_TX;
    hi2s1.Init.Standard = I2S_STANDARD_PHILIPS;
    hi2s1.Init.DataFormat = I2S_DATAFORMAT_16B;
    hi2s1.Init.MCLKOutput = I2S_MCLKOUTPUT_DISABLE;
    hi2s1.Init.AudioFreq = I2S_AUDIOFREQ_16K;
    hi2s1.Init.CPOL = I2S_CPOL_LOW;
    hi2s1.Init.FirstBit = I2S_FIRSTBIT_MSB;
    hi2s1.Init.WSInversion = I2S_WS_INVERSION_DISABLE;
    hi2s1.Init.Data24BitAlignment = I2S_DATA_24BIT_ALIGNMENT_RIGHT;
    hi2s1.Init.MasterKeepIOState = I2S_MASTER_KEEP_IO_STATE_DISABLE;
    
    if (HAL_I2S_Init(&hi2s1) != HAL_OK)
    {
        Error_Handler();
    }
}

/* CubeMX外设初始化 */
int cubemx_peripheral_init(void)
{
    MX_GPDMA1_Init();
    MX_I2S1_Init();
    MX_ADC1_Init();
    return 0;
}
INIT_BOARD_EXPORT(cubemx_peripheral_init);

