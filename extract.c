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

typedef unsigned char uchar;

uchar getBitAt(uchar byte, uchar index) {
    return (byte >> index) & 1;
}

void setBitAt(uchar* byte, uchar index, uchar bit) {
    bit &= 1; // On s'assure que seul le 1er bit est différent de 0
    const uchar mask = 1 << index;
    *byte &= ~mask;
    *byte |= bit << index;
}

char* extractBytes(uchar* img, long byteCount, uchar byteChunkSize, long offset) {
    const uchar lastBitsMask = 
        byteChunkSize == 8 ? 0b11111111 : 
        byteChunkSize == 4 ? 0b00001111 : 
        byteChunkSize == 2 ? 0b00000011 : 
        byteChunkSize == 1 ? 0b00000001 : 0;

    char* buffer = calloc(byteCount, sizeof(char));
    long bufferIndex = 0;
    char bytePos = 0;

    // imgIndex est statique pour que la progression dans l'image soit conservée d'appel en appel
    long imgIndex = offset;
    
    // Le nombre de byteChunk = le nombre de bytes multiplié par le nombre de byteChunk dans 1 byte
    int byteChunkCount = byteCount * (8 / byteChunkSize);
    long targetImgIndex = imgIndex + byteChunkCount;

    for (; imgIndex < targetImgIndex; imgIndex++) {
        // le byte correspondant à la valeur du composant de pixel
        char imageByte = img[imgIndex];
        // On ne veut que les derniers bits
        char byteChunk = imageByte & lastBitsMask;
        // On décale le byteChunk pour qu'il soit au bon endroit dans le byte
        byteChunk <<= 8 - bytePos - byteChunkSize;
        // On ajoute le byteChunk au byte courant du buffer
        // (on est assurés que byteChunk ne dépasse pas le nombre de bits de byteChunkSize)
        buffer[bufferIndex] |= byteChunk;

        bytePos += byteChunkSize;
        if (bytePos >= 8) {
            bytePos = 0;
            bufferIndex++;
        }
    }

    return buffer;
}

int main() {
    // === Lecture des informations de l'image ===

    int width, height, channels;
    uchar *img = stbi_load(IMG_PATH, &width, &height, &channels, USED_CHANNELS);
    if (img == NULL) {
        printf("Error in loading the image\n");
        return 1;        
    }

    printf("Source image: %s%s%s\n", COLOR, IMG_PATH, RESET);
    printf("Size: %d x %d px\n", width, height);
    printf("Used channels: %d / %d\n", USED_CHANNELS, channels);

    // Lecture du byteChunkSizeMode et calcul de byteChunkSize

    uchar byteChunkSizeMode = 0;
    setBitAt(&byteChunkSizeMode, 0, getBitAt(img[0], 0));
    setBitAt(&byteChunkSizeMode, 1, getBitAt(img[1], 0));

    const uchar byteChunkSize = pow(2, byteChunkSizeMode);
    if (8 % byteChunkSize != 0) {
        printf("Byte chunk size must be a divisor of 8, but found %d\n", byteChunkSize);
        return 1;
    }

    printf("\n");
    printf("Byte chunk size: %d bit\n", byteChunkSize);

    // === Extraction du fichier ===
    // Note: Le prefix est la zone mémoire où on écrit la valeur de filelen

    long imgPos = 2; // On commence au 2e composant de l'image

    long filelen; // La taille du fichier en bytes
    char prefixlen = sizeof(filelen); // La taille du prefix en bytes
    char *prefixBuffer = extractBytes(img, prefixlen, byteChunkSize, imgPos); // On extrait la valeur du préfix sous forme de buffer de char
    filelen = *((long*) prefixBuffer); // On assigne la valeur (il faut caster le pointeur puis déréférencer)
    imgPos += prefixlen * (8 / byteChunkSize); // On déplace la position de lecture dans l'image

    char *buffer = extractBytes(img, filelen, byteChunkSize, imgPos); // On extrait le fichier

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
    printf("Size: %ld bytes\n", filelen);

    // On libère la mémoire
    free(prefixBuffer);
    free(buffer);
    stbi_image_free(img);
}