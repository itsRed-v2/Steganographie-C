#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "stb_image.h"
#include "stb_image_write.h"

#define COLOR "\e[38;5;4m" // Blue
#define RESET "\e[m"

#define USED_CHANNELS 3

#define IMG_PATH "files/clouds.png"
#define FILE_PATH "files/landscape.png"
#define OUTPUT_PATH "output/out.png"

/*\
 * Modes disponibles:
 * 0: utilisation de 1 bit par composant de pixel (pratiquement indétectable)
 * 1: utilisation de 2 bit par composant de pixel (difficilement visible)
 * 2: utilisation de 4 bit par composant de pixel (très visible)
 * 3: utilisation de 8 bit par composant de pixel (l'image d'origine est complètement écrasée)
\*/
#define BYTE_CHUNK_SIZE_MODE 2

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

void writeBufferToImg(uchar* img, uchar* buffer, const long bufferLength, const uchar byteChunkSize) {
    const uchar lastBitsMask = 
        byteChunkSize == 8 ? 0b11111111 : 
        byteChunkSize == 4 ? 0b00001111 : 
        byteChunkSize == 2 ? 0b00000011 : 
        byteChunkSize == 1 ? 0b00000001 : 0;

    // Le nombre de byteChunk = le nombre de bytes multiplié par le nombre de byteChunk dans 1 byte
    const long byteChunkCount = bufferLength * (8 / byteChunkSize);

    long bufferIndex = 0;
    uchar bytePos = 0;
    for (long imageIndex = 0; imageIndex < byteChunkCount; imageIndex++) {

        // On lit le byte courant du buffer
        uchar byte = buffer[bufferIndex];
        // On décale le byteChunk pour qu'il soit positionné au début du byte
        uchar byteChunk = byte >> (8 - bytePos - byteChunkSize);
        // On met à 0 les éventuels bits qui sont à gauche du byteChunk
        byteChunk &= lastBitsMask;

        // On incrémente la position du byteChunk dans le byte
        bytePos += byteChunkSize;
        // Si bytePos est à 8, on passe au prochain byte
        if (bytePos == 8) {
            bytePos = 0;
            bufferIndex++;
        }

        // le byte correspondant à la valeur du composant de pixel
        uchar imageByte = img[imageIndex];
        // On met à zéro les derniers bits du byte avec le mask
        imageByte &= ~lastBitsMask;
        // On ajoute le byteChunk à la place des bits qui ont été mis à zéro
        // (on est assurés que byteChunk ne dépasse pas le nombre de bits de byteChunkSize
        imageByte |= byteChunk;
        // On réassigne le nouveau byte modifié à l'image
        img[imageIndex] = imageByte;
    }
}

int main() {
    const uchar byteChunkSize = pow(2, BYTE_CHUNK_SIZE_MODE);
    if (8 % byteChunkSize != 0) {
        printf("Byte chunk size must be a divisor of 8, but found %d\n", byteChunkSize);
        return 1;
    }

    printf("Byte chunk size: %d bit\n", byteChunkSize);

    // Lecture des informations de l'image

    int width, height, channels;
    uchar *img = stbi_load(IMG_PATH, &width, &height, &channels, USED_CHANNELS);
    if (img == NULL) {
        printf("Error in loading the image\n");
        return 1;
    }

    printf("\n");
    printf("Base image: %s%s%s\n", COLOR, IMG_PATH, RESET);
    printf("Size: %d x %d px\n", width, height);
    printf("Used channels: %d / %d\n", USED_CHANNELS, channels);

    // Ecriture du BYTE_CHUNK_SIZE_MODE sur les 2 premiers éléments de l'image

    setBitAt(img, 0, getBitAt(BYTE_CHUNK_SIZE_MODE, 0));
    setBitAt(img + 1, 0, getBitAt(BYTE_CHUNK_SIZE_MODE, 1));

    // Lecture du fichier

    FILE *file;
    long filelen; // Le nombre d'octets dans le fichier

    file = fopen(FILE_PATH, "rb"); // on ouvre le fichier en mode rb: read binary
    if (file == NULL) {
        printf("Error in reading the file: %s", FILE_PATH);
        return 1;
    }
    fseek(file, 0, SEEK_END); // On place la tête de lecture à la fin du fichier
    filelen = ftell(file); // ftell renvoie la position de la tête de lecture (ici, la longueur du fichier)
    rewind(file); // on remet la tête au début du fichier

    // Note: Le prefix est la zone mémoire où on écrit la valeur de filelen
    char prefixlen = sizeof(filelen); // La taille du prefix en bytes
    long bufferlen = filelen + prefixlen; // La taille du buffer en bytes
    uchar *buffer = malloc(bufferlen * sizeof(char));

    // Le fichier commence <prefixlen> bytes après le buffer
    uchar *fileBytes = buffer + prefixlen;
    // fread: lecture du contenu de <file> en <1> bloc de longueur <filelen>,
    // et stockage du resultat dans le buffer <fileBytes>
    fread(fileBytes, filelen, 1, file);

    // On convertir le buffer (pointeur de char) en pointeur de long pour 
    // pouvoir écrire la valeur de filelen sur les premiers bytes du buffer
    long * filelenPointer = (long*) buffer;
    *filelenPointer = filelen;

    // On affiche les infos du fichier à encoder
    printf("\n");
    printf("Target file: %s%s%s\n", COLOR, FILE_PATH, RESET);
    printf("Size: %ld bytes\n", filelen);

    // Le nombre de composants de pixels dans l'image
    int imgSize = width * height * USED_CHANNELS;
    // Le nombre de byteChunk = le nombre de bytes multiplié par le nombre de byteChunk dans 1 byte
    long byteChunkCount = bufferlen * (8 / byteChunkSize);
    // On s'assure que l'image est assez grande pour contenir tous les byteChunk
    // (Chaque composant de pixel peut contenir 1 byteChunk)
    // On enlève 2 à imgSize car 2 composants sont révervés pour écrire BYTE_CHUNK_SIZE_MODE
    if (imgSize - 2 < byteChunkCount) {
        printf("The image is too small to contain this file !");
        return 1;
    }

    printf("\n");
    printf("Processing...\n");

    // On écrit à partir du 2e élément de img car les deux premiers sont réservés pour BYTE_CHUNK_SIZE_MODE
    writeBufferToImg(img + 2, buffer, bufferlen, byteChunkSize);

    printf("Writing the resulting image to output file...\n");
    // On écrit l'image au format png:
    // OUTPUT_PATH: le chemin où on enregistre l'image; width et height: la taille de l'image
    // USED_CHANNELS: le nombre de channels utilisés dans l'image enregistrée
    // img: le buffer contenant l'image
    // width * USED_CHANNELS: la taille (en bytes) d'une ligne de pixels sur l'image
    stbi_write_png(OUTPUT_PATH, width, height, USED_CHANNELS, img, width * USED_CHANNELS);

    printf("Done.\n");
    printf("Output image: %s%s%s\n", COLOR, OUTPUT_PATH, RESET);

    // On libère la mémoire
    free(buffer);
    stbi_image_free(img);
}