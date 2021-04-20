#include "parser_util.h"

/*
HEX: 14 AAAA AAAA CD00 LLLL CH
AA = Address
CD = Checksum of Data
LL = Length of Data
CH = Checksum of Header
*/


//FILE *oout;

int search_data(FILE *file) {
    int ret = -1;
    int skip = 0;
    uint16_t length = 0x0A;

    unsigned char *buffer = (char*)malloc(length);
    if (buffer == NULL) {
        ret = -2;
        goto exit;
    }

    while (1) {
        size_t size = fread(buffer, 1, length, file);
        if (size < length) {
            ret = -3;
            goto exit;
        }

        // basic check
        if ((buffer[0] != 0x14) || (buffer[6] != 0x00)) {
            goto failed;
        }

        // compute checksum
        uint8_t checksum = 0;
        int i = 0;
        for (i = 1; i < length - 1; i++)
            checksum += buffer[i];

        if ((checksum^0xFF) != buffer[9]) {
            goto failed;
        }

success:
        fseek(file, -length, SEEK_CUR);
        ret = 0;
        goto exit;

failed:
        skip ++; // skip buffer[0]
        //fwrite(buffer, 1, 1, oout);
        for (i = 1; i < length; i++) {
            if(buffer[i] == 0x14) {
                fseek(file, i-length, SEEK_CUR);
                break; // for(;;)
            } else {
                skip ++; // skip buffer[i]
                //fwrite(buffer+i, 1, 1, oout);
            }
        }
    };

exit:
    if (skip) {
        printf("skip %x\n", skip);
    }
    if (buffer)
        free(buffer);
    return ret;
}

int main(int argc, char **argv) {
	FILE *fp_mcu = NULL, *fp_out = NULL;
	fp_mcu  = fopen((argc > 1) ? argv[1] : "NHL105530.c1", "rb");
	fp_out = fopen((argc > 2) ? argv[2] : NULL, "wb");
	//oout = fopen("DUMMY", "wb");

    uint32_t size_out = 0;

	if (fp_mcu == NULL)
        exit(EXIT_FAILURE);

    printf("FileID:\t\t");
    if (read8(fp_mcu) != 0xA2)
        goto exit;

    uint32_t header_size = 0;
    printf("\nHeader size:\t");
    if ((header_size = read32(fp_mcu)) <= 0)
        goto exit;

    uint32_t tokens = 0;
    printf("\nTokens:\t\t");
    if ((tokens = read32(fp_mcu)) <= 0)
        goto exit;

    int i = 0;
    for (i = 0; i < tokens; i++) {
        printf("\nToken[%x]:\t", i);

        uint8_t token_type = 0;
        if ((token_type = read8(fp_mcu)) <= 0)
            goto exit;
        printf("len=");

        uint8_t token_size = 0;
        if ((token_size = read8(fp_mcu)) <= 0)
            goto exit;

        switch (token_type) {
            case 0xc2:
            case 0xc3:
                printf("section [ ");
                if (read_len(fp_mcu, token_size, 'c') < 0)
                    goto exit;
                printf("]");
                break;
            case 0xd3:
                printf("signature [ ");
                if (read_len(fp_mcu, token_size, 'x') < 0)
                    goto exit;
                printf("]");
                break;
            case 0xd4:
                printf("supportedIds [ ");
                int supId = 0;
                for (supId = 0; supId < token_size; supId += 2) {
                    if (read16(fp_mcu) <= 0)
                        goto exit;
                    printf(" ");
                }
                printf("]");
                break;
            case 0xc8:
                printf("areas [ ");
                int area = 0;
                for (area = 0; area < token_size; area += 4) {
                    uint32_t sz = read32(fp_mcu);
                    if (sz > size_out)
                        size_out = sz;

                    if ((area % 8) == 0)
                        printf("-");
                    else
                        printf("\n\t\t\t\t");
                }
                printf("]");
                break;
            default:
                printf("[ ");
                if (read_len(fp_mcu, token_size, 'x') < 0)
                    goto exit;
                printf("]");
                break;
        }
    }

    printf("\n");
    if (search_data(fp_mcu))
        goto exit;

    do {
        uint16_t header_size = 0x0A;
        unsigned char *buffer = (char*)malloc(header_size);
        if (buffer == NULL) {
            goto exit;
        }

        size_t size = fread(buffer, 1, header_size, fp_mcu);
        if (size < header_size) {
            free(buffer);
            goto exit;
        }

        if (buffer[0] == 0x14 && buffer[6] == 0x00) {
            uint32_t offset = buffer[1] <<8*3 | buffer[2] << 8*2 | buffer[3] << 8*1 | buffer[4];
            uint32_t length = buffer[7] << 8*1 | buffer[8];
            printf("src %04lx; offset %08x; length: %04x\n", ftell(fp_mcu)-header_size, offset, length);

            if (fp_out == NULL) {
                free(buffer);
                fseek(fp_mcu, length, SEEK_CUR);
                continue;
            }

            // allocate memory
            char *data = (char*) malloc(length);
            if (data == NULL) {
                free(buffer);
                goto exit;
            }

            // read data
            if (fread(data, 1, length, fp_mcu) < length) {
                free(buffer);
                free(data);
                goto exit;
            }

            // set write position
            if (fseek(fp_out, offset, SEEK_SET) != 0) {
                free(buffer);
                free(data);
                goto exit;
            }

            // write data
            if (fwrite(data, 1, length, fp_out) < length) {
                free(buffer);
                free(data);
                goto exit;
            }

            free(data);
            free(buffer);
        } else {
            if (search_data(fp_mcu)) {
                free(buffer);
                goto exit;
            }
        }
    } while (1);


exit:

	if(fp_out)
		fclose(fp_out);
	if(fp_mcu)
		fclose(fp_mcu);
	exit(EXIT_SUCCESS);
}
