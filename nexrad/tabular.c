#include <stdlib.h>

#include <nexrad/tabular.h>

nexrad_tabular_text *nexrad_tabular_block_open(nexrad_tabular_block *block) {
    nexrad_tabular_text *text;

    if (block == NULL) return NULL;

    if ((text = malloc(sizeof(*text))) == NULL) {
        goto error_malloc;
    }

    text->current    = (char *)block + sizeof(nexrad_tabular_block);
    text->page       = 1;
    text->line       = 1;
    text->pages_left = be16toh(block->pages);
    text->bytes_left = be32toh(block->header.size);

    return text;

error_malloc:
    return NULL;
}

ssize_t nexrad_tabular_block_read_line(nexrad_tabular_text *block, char **data, int *page, int *line) {
    size_t chars;

    if (block == NULL) return -1;

    /*
     * Return a zero if there are no pages or bytes left in the tabular block.
     */
    if (block->pages_left == 0 || block->bytes_left == 0) {
        return 0;
    }

    /*
     * Check and see if we have arrived at an end-of-page flag.  Also, if we
     * have finished reading the final page, return 0 to indicate the last line
     * has been read.
     */
    if ((int16_t)be16toh(*((int16_t *)block->current)) == -1) {
        block->page++;
        block->line = 1;

        if (block->pages_left-- == 0) {
            return 0;
        }

        /*
         * Increment the current pointer beyond the end-of-page flag.
         */
        block->current = (char *)block->current + sizeof(int16_t);
    }

    /*
     * Detect the number of characters in the current line.  If this number
     * seems implausible, then return -1 to indicate an error.
     */
    if ((chars = (size_t)be16toh(*((uint16_t *)block->current))) > NEXRAD_TABULAR_BLOCK_MAX_LINE_SIZE) {
        return -1;
    }

    /*
     * If the caller has provided a pointer to an address to store the start of
     * the text line, then populate that.
     */
    if (data != NULL) {
        *data = block->current + sizeof(uint16_t);
    }

    /*
     * If the caller has provided a pointer to an address to store the current
     * page number, then supply that.
     */
    if (page != NULL) {
        *page = block->page;
    }

    /*
     * If the caller has provided a pointer to an address to store the current
     * line number, then supply that.
     */
    if (line != NULL) {
        *line = block->line;
    }

    /*
     * Increment the current line number.
     */
    block->line++;

    /*
     * Advance the current data pointer beyond the current line.
     */
    block->current = (char *)block->current + sizeof(uint16_t) + chars;

    /*
     * Return the number of characters in the current line.
     */
    return chars;
}

void nexrad_tabular_block_close(nexrad_tabular_text *block) {
    if (block == NULL) return;

    block->current    = NULL;
    block->page       = 0;
    block->line       = 0;
    block->pages_left = 0;
    block->bytes_left = 0;

    free(block);
}
