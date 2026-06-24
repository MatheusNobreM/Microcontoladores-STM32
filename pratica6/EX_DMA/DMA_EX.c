	/*Global Variable*/
	
	uint16_t adc_data[3];
	
	/*Configure DMA*/

	/*Enable Clock access to DMA1*/
	RCC->AHBENR|=RCC_AHBENR_DMA1EN;

	/*DMA1_Channel1 is for ADC1*/

	DMA1_Channel1->CCR = 0x000005A0;

	DMA1_Channel1->CNDTR=3;

	DMA1_Channel1->CPAR=(uint32_t)(&ADC1->DR);

	DMA1_Channel1->CMAR=(uint32_t)(adc_data);

	DMA1_Channel1->CCR|=DMA_CCR_EN;
