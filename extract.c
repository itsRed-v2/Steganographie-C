#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "stb_image.h"
#include "stb_image_write.h"

#define COLOR "\e[38;5;4m" // Blue
#define RESET "\e[m"

#define USED_CHANNELS 3

#define IMG_PATH "output/out.png"
#define OUTPUT_PATH "extracted/out.png"
#define BYTE_CHUNK_SIZE 4

#define lastBitsMask (\
BYTE_CHUNK_SIZE == 8 ? 0b11111111 : \
BYTE_CHUNK_SIZE == 4 ? 0b00001111 : \
BYTE_CHUNK_SIZE == 2 ? 0b00000011 : \
BYTE_CHUNK_SIZE == 1 ? 0b00000001 : 0 \
)

long extractedFileSize = 226933;

void printBinary(char *prefix, char byte) {
    char binaryString[] = "00000000";    
    for (int i = 0; i < 8; i++) {
        if (byte & ((char) pow(2, 7-i)))
            binaryString[i] = '1';
    }
    printf("%s%s\n", prefix, binaryString);
}

char* extractBytes(unsigned char* img, long byteCount) {
    // Le nombre de byteChunk = le nombre de bytes multiplié par le nombre de byteChunk dans 1 byte
    int byteChunkCount = byteCount * (8 / BYTE_CHUNK_SIZE);

    char* buffer = calloc(byteCount, sizeof(char));

    long imgIndex = 0;
    long bufferIndex = 0;
    char bytePos = 0;

    for (imgIndex = 0; imgIndex < byteChunkCount; imgIndex++) {
        // le byte correspondant à la valeur du composant de pixel
        char imageByte = img[imgIndex];
        // On ne veut que les derniers bits
        char byteChunk = imageByte & lastBitsMask;
        // On décale le byteChunk pour qu'il soit au bon endroit dans le byte
        byteChunk <<= 8 - bytePos - BYTE_CHUNK_SIZE;
        // On ajoute le byteChunk au byte courant du buffer
        // (on est assurés que byteChunk ne dépasse pas le nombre de bits de BYTE_CHUNK_SIZE)
        buffer[bufferIndex] |= byteChunk;

        bytePos += BYTE_CHUNK_SIZE;
        if (bytePos >= 8) {
            bytePos = 0;
            bufferIndex++;
        }
    }

    return buffer;
}

int main() {
    if (8 % BYTE_CHUNK_SIZE != 0) {
        printf("Byte chunk size must be a divisor of 8, but found %d\n", BYTE_CHUNK_SIZE);
        return 1;
    }

    // === Reading image data ===

    int width, height, channels;
    unsigned char *img = stbi_load(IMG_PATH, &width, &height, &channels, USED_CHANNELS);
    if (img == NULL) {
        printf("Error in loading the image\n");
        return 1;        
    }

    printf("Source image: %s%s%s\n", COLOR, IMG_PATH, RESET);
    printf("Size: %d x %d px\n", width, height);
    printf("Used channels: %d / %d\n", USED_CHANNELS, channels);

    // Le nombre de composants de pixels dans l'image
    // int imgSize = width * height * USED_CHANNELS;

    // buffer = malloc(extractedFileSize * sizeof(char));

    printf("\n");
    printf("Output file: %s%s%s\n", COLOR, OUTPUT_PATH, RESET);
    printf("Size: %ld bytes\n", extractedFileSize);

    // Le nombre de byteChunk = le nombre de bytes multiplié par le nombre de byteChunk dans 1 byte
    // int byteChunkCount = extractedFileSize * (8 / BYTE_CHUNK_SIZE);

    // On s'assure que l'image est assez grande pour contenir tous les byteChunk
    // (Chaque composant de pixel peut contenir 1 byteChunk) 
    // if (imgSize < byteChunkCount) {
    //     printf("The image is too small to contain this file !");
    //     return 1;
    // }

    printf("\n");
    printf("Processing...\n");

    char *buffer = extractBytes(img, extractedFileSize);

    // === Writing file ===

    printf("Writing the result to output file...\n");

    // on ouvre le fichier en mode wb: write binary
    FILE *file = fopen(OUTPUT_PATH, "wb");
    // fwrite: écriture du contenu du <buffer> en <1> bloc de longueur <extractedFileSize>,
    // et stockage du resultat dans le fichier <file>
    fwrite(buffer, extractedFileSize, 1, file);

    printf("Done.\n");
    printf("Output file: %s%s%s\n", COLOR, OUTPUT_PATH, RESET);

    // On libère la mémoire
    free(buffer);
    stbi_image_free(img);
}