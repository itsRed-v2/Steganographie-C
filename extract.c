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

void printBinary(char *prefix, char byte) {
    char binaryString[] = "00000000";    
    for (int i = 0; i < 8; i++) {
        if (byte & ((char) pow(2, 7-i)))
            binaryString[i] = '1';
    }
    printf("%s%s\n", prefix, binaryString);
}

char* extractNextBytes(unsigned char* img, long byteCount) {
    char* buffer = calloc(byteCount, sizeof(char));
    long bufferIndex = 0;
    char bytePos = 0;

    // imgIndex est statique pour que la progression dans l'image soit conservée d'appel en appel
    static long imgIndex = 0;
    
    // Le nombre de byteChunk = le nombre de bytes multiplié par le nombre de byteChunk dans 1 byte
    int byteChunkCount = byteCount * (8 / BYTE_CHUNK_SIZE);
    long targetImgIndex = imgIndex + byteChunkCount;

    for (; imgIndex < targetImgIndex; imgIndex++) {
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

    // === Lecture des informations de l'image ===

    int width, height, channels;
    unsigned char *img = stbi_load(IMG_PATH, &width, &height, &channels, USED_CHANNELS);
    if (img == NULL) {
        printf("Error in loading the image\n");
        return 1;        
    }

    printf("Source image: %s%s%s\n", COLOR, IMG_PATH, RESET);
    printf("Size: %d x %d px\n", width, height);
    printf("Used channels: %d / %d\n", USED_CHANNELS, channels);

    // === Extraction du fichier ===
    // Note: Le prefix est la zone mémoire où on écrit la valeur de filelen

    long filelen; // La taille du fichier en bytes
    char prefixlen = sizeof(filelen); // La taille du prefix en bytes
    char *prefixBuffer = extractNextBytes(img, prefixlen); // On extrait la valeur du préfix sous forme de buffer de char
    filelen = *((long*) prefixBuffer); // On assigne la valeur (il faut caster le pointeur puis déréférencer)

    printf("\n");
    printf("Output file: %s%s%s\n", COLOR, OUTPUT_PATH, RESET);
    printf("Size: %ld bytes\n", filelen);

    // On commence l'extraction du fichier <prefixLen> bytes après le début de l'image
    // unsigned char *fileStart = img + (prefixlen * (8 / BYTE_CHUNK_SIZE));
    char *buffer = extractNextBytes(img, filelen);

    // === Ecriture du fichier décodé ===
    
    printf("\n");
    printf("Writing the result to output file...\n");

    // on ouvre le fichier en mode wb: write binary
    FILE *file = fopen(OUTPUT_PATH, "wb");
    // fwrite: écriture du contenu du <buffer> en <1> bloc de longueur <extractedFileSize>,
    // et stockage du resultat dans le fichier <file>
    fwrite(buffer, filelen, 1, file);

    printf("Done.\n");
    printf("Output file: %s%s%s\n", COLOR, OUTPUT_PATH, RESET);

    // On libère la mémoire
    free(prefixBuffer);
    free(buffer);
    stbi_image_free(img);
}