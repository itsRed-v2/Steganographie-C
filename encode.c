#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "stb_image.h"
#include "stb_image_write.h"

#define COLOR "\e[38;5;4m" // Blue
#define RESET "\e[m"

#define USED_CHANNELS 3

#define IMG_PATH "files/clouds.png"
#define FILE_PATH "files/dice-transparent.png"
#define OUTPUT_PATH "output/out.png"
#define BYTE_CHUNK_SIZE 4

#define lastBitsMask (\
BYTE_CHUNK_SIZE == 8 ? 0b11111111 : \
BYTE_CHUNK_SIZE == 4 ? 0b00001111 : \
BYTE_CHUNK_SIZE == 2 ? 0b00000011 : \
BYTE_CHUNK_SIZE == 1 ? 0b00000001 : 0 \
)

char *fileBytes;

void printBinary(char *prefix, char byte) {
    char binaryString[] = "00000000";    
    for (int i = 0; i < 8; i++) {
        if (byte & ((char) pow(2, 7-i)))
            binaryString[i] = '1';
    }
    printf("%s%s\n", prefix, binaryString);
}

char nextByteChunk() {
    static int index = 0;
    static int bytePos = 0;

    char byteChunk = fileBytes[index];
    byteChunk >>= 8 - bytePos - BYTE_CHUNK_SIZE;
    byteChunk &= lastBitsMask;

    bytePos += BYTE_CHUNK_SIZE;
    if (bytePos == 8) {
        bytePos = 0;
        index++;
    }

    return byteChunk;
}

int main() {
    if (8 % BYTE_CHUNK_SIZE != 0) {
        printf("Byte chunk size must be a divisor of 8, but found %d\n", BYTE_CHUNK_SIZE);
        return 1;
    }

    // Reading image data

    int width, height, channels;
    unsigned char *img = stbi_load(IMG_PATH, &width, &height, &channels, USED_CHANNELS);
    if (img == NULL) {
        printf("Error in loading the image\n");
        return 1;        
    }

    printf("Base image: %s%s%s\n", COLOR, IMG_PATH, RESET);
    printf("Size: %d x %d px\n", width, height);
    printf("Used channels: %d / %d\n", USED_CHANNELS, channels);

    // Le nombre de composants de pixels dans l'image
    int imgSize = width * height * USED_CHANNELS;

    // Reading file data

    FILE *file;
    long filelen; // Le nombre d'octets dans le fichier

    file = fopen(FILE_PATH, "rb"); // on ouvre le fichier en mode rb: read binary
    fseek(file, 0, SEEK_END); // On place la tête de lecture à la fin du fichier
    filelen = ftell(file); // ftell renvoie la position de la tête de lecture (ici, la longueur du fichier)
    rewind(file); // on remet la tête au début du fichier

    fileBytes = malloc(filelen * sizeof(char));
    // fread: lecture du contenu de <file> en <1> bloc de longueur <filelen>,
    // et stockage du resultat dans le buffer <fileBytes>
    fread(fileBytes, filelen, 1, file);

    printf("\n");
    printf("Target file: %s%s%s\n", COLOR, FILE_PATH, RESET);
    printf("Size: %ld bytes\n", filelen);

    // Le nombre de byteChunk = le nombre de bytes multiplié par le nombre de byteChunk dans 1 byte
    int byteChunkCount = filelen * (8 / BYTE_CHUNK_SIZE);

    // On s'assure que l'image est assez grande pour contenir tous les byteChunk
    // (Chaque composant de pixel peut contenir 1 byteChunk) 
    if (imgSize < byteChunkCount) {
        printf("The image is too small to contain this file !");
        return 1;
    }

    printf("\n");
    printf("Processing...\n");

    for (int i = 0; i < byteChunkCount; i++) {
        // On lit le prochain byteChunk du message à encoder
        char byteChunk = nextByteChunk();
        // le byte correspondant à la valeur du composant de pixel
        char imageByte = img[i];
        // On met à zéro les derniers bits du byte avec le mask
        imageByte &= ~lastBitsMask;
        // On ajoute le byteChunk à la place des bits qui ont été mis à zéro
        // (on est assurés que byteChunk ne dépasse pas le nombre de bits de BYTE_CHUNK_SIZE)
        imageByte |= byteChunk;
        // On réassigne le nouveau byte modifié à l'image
        img[i] = imageByte;
    }

    printf("Writing the resulting image to output file...\n");
    // On écrit l'image au format png:
    // OUTPUT_PATH: le chemin où on enregistre l'image; width et height: la taille de l'image
    // USED_CHANNELS: le nombre de channels utilisés dans l'image enregistrée (3 pour rgb, 4 pour rgba)
    // img: le buffer contenant l'image
    // width * USED_CHANNELS: la taille (en bytes) d'une ligne de pixels sur l'image
    stbi_write_png(OUTPUT_PATH, width, height, USED_CHANNELS, img, width * USED_CHANNELS);

    printf("Done.\n");
    printf("Output image: %s%s%s\n", COLOR, OUTPUT_PATH, RESET);

    // On libère la mémoire
    free(fileBytes);
    stbi_image_free(img);
}