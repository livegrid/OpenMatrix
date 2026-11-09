/******************************************************************************************
 * @file        spi_dma_seg_tx_payload.c
 * @author      github.com/mrcodetastic
 * @date        2024
 * @brief       ESP32-S3 implementation for a MBI5135 PWM chip based LED Matrix Panel
 ******************************************************************************************/
#pragma once

/******** GCLK and row address payload for the octal SPI loop **********
 * Each row-sequence is REPEATS x a three byte sequence. Only the third byte has
 * the MSB (GCLK) set. The row index (0..19) sits in bits 2..6 (address lines A..E)
 * of every byte. Each row-sequence is wrapped in PADDING_SIZE bytes of padding.
 */

const int BYTES_PER_REPEAT  = 3; // gclk pulse on 3rd byte, gives us enough time to send frame data then.
const int REPEATS           = 513; // 513 gclks per the documentation
const int SEQUENCE_SIZE     = BYTES_PER_REPEAT * REPEATS;
const int REPEAT_COUNT      = 20; // 20 rows
const int PADDING_SIZE      = 8; // some delay between row changes?
const int GCLK_TOTAL_SIZE   = (PADDING_SIZE + SEQUENCE_SIZE + PADDING_SIZE) * REPEAT_COUNT;

// GCLK_TOTAL_SIZE = 31100 which is just under the SPI max transfer size of 32kB per SEGMENT

DMA_ATTR static uint8_t *spi_tx_octal_payload   = NULL; // data for gclk

// Generate MBI5153 GCLK DATA for Byte 0 of parallel output.
// Function to calculate the value of a byte at a specific position
uint8_t getByteValue(int position) {
    int sequenceIndex = position / (PADDING_SIZE + SEQUENCE_SIZE + PADDING_SIZE);
    int offset = position % (PADDING_SIZE + SEQUENCE_SIZE + PADDING_SIZE);

    // Calculate position within the sequence
    int sequencePosition = offset - PADDING_SIZE;
    int byteIndex = sequencePosition % BYTES_PER_REPEAT;

    uint8_t repeatValue = sequenceIndex & 0x1F;  // Repeat value embedded in the LSBs

    // Handle padding
    if (offset < PADDING_SIZE || offset >= PADDING_SIZE + SEQUENCE_SIZE) {
		// fix slight gclk sync out of sync on ESP32S3... causes gclk to get out of sync with rowscan and image to slide around
		return 0x80 | (repeatValue << 2);	
    }    

    // Calculate the byte value based on its position in the repeat
    if (byteIndex == 2) {
        return 0x80 | (repeatValue << 2);  // Third byte with MSB set
    } else {
        return (repeatValue << 2);  // First and second bytes
    }
}

void allocate_gclk_dma_memory()
{
	// We don't want to do this twice
	assert(spi_tx_octal_payload == NULL);

	size_t alloc_size_bytes = (GCLK_TOTAL_SIZE) * sizeof(uint8_t);

	ESP_LOGI("spi_dma_seg_tx_payload", "Allocating SRAM DMA memory for global_buffer_gclk_cdata.");
	spi_tx_octal_payload = (uint8_t *)(heap_caps_malloc(alloc_size_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA));
	assert(spi_tx_octal_payload != NULL);

	ESP_LOGI("spi_dma_seg_tx_payload", "GCLK data total size: %d bytes.", GCLK_TOTAL_SIZE);

	// Populate based on algo
	for (int i = 0; i < alloc_size_bytes; i++) {
		spi_tx_octal_payload[i] = getByteValue(i);
	}
} // end allocate_gclk_dma_memory
