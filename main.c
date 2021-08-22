#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <sys/stat.h>
#include <errno.h>
#include <getopt.h>

#include <fenv.h>
#include <float.h>
#define  _GNU_SOURCE

#include <math.h>

#define IN_ASCII_RANGE(num)     ((num) > 0x1f && (num) < 0x7e)

#define _BV(x)      (1 << (x))
#define NO_FLAGS    0x00
#define BAD_FLAGS   0xFF
#define E_FLAG      _BV(0)      // Display entropy
#define P_FLAG      _BV(1)      // Display Probabilities
#define O_FLAG      _BV(2)      // Display occurrencies
#define G_FLAG      _BV(3)      // Graph probabilities
#define V_FLAG      _BV(4)      // Verbose

int feenableexcept(int excepts);

uint32_t occurrence[0xFF];
double probability[0xFF];

void die(char *msg)
{
    if (errno) {
        perror(msg);
    } else {
        fputs(msg, stderr);
        fputc('\n', stderr);
    }

    exit(-1);
}

_Bool bad_file(FILE *file)
{
    int fd;
    struct stat st;

    fd = fileno(file);
    if (fstat(fd, &st) == -1)
        return 1;

    if (S_ISREG(st.st_mode))
        return 0;

    return 1;
}

void count_occurrence(uint8_t *buffer, uint8_t sz)
{
    for (; sz != 0; --sz)
        occurrence[buffer[sz - 1]]++;
}

int get_distribution(FILE *file)
{
    int total_size = 0;
    uint8_t sz, err = 0;
    uint8_t buffer[0xFF];

    while (!feof(file) || (err = ferror(file))) {
        sz = fread(buffer, 1, 0xFF, file);
        total_size += sz;

        count_occurrence(buffer, sz);
    }

    if (err)
        return -1;

    return total_size;
}

void compute_probability(uint32_t sz)
{
    for (uint8_t i = 0; i < 0xFF; i++)
        probability[i] = (double) occurrence[i] / sz;
}

double entropy()
{
    double ent = 0;

    for (uint8_t i = 0; i < 0xFF; i++)
        // lim (x -> 0) (x ln(x)) = 0
        if (probability[i])
            ent += probability[i] * log2(probability[i]);
    
    return -ent;
}

void display_occurrencies()
{
    for (uint8_t i = 0; i < 0xFF; i++)
        printf("%2X (%c):\t%d\n", i,
                                 IN_ASCII_RANGE(i) ? i : ' ',
                                 occurrence[i]);
}

char *graph(char* buffer, double prob, unsigned int scale)
{
    uint8_t i;

    for (i = 0; i < prob * scale; i++)
        buffer[i] = '*';
    buffer[i] = 0;

    return buffer;
}

int display_probabilities(unsigned int scale, _Bool graphical)
{
    char *buff;

    buff = malloc(scale + 1);
    if (!buff)
        return -1;

    for (uint8_t i = 0; i < 0xFF; i++)  {
        printf("%2X (%c):\t", i, IN_ASCII_RANGE(i) ? i : ' ');

        if (graphical)
            puts(graph(buff, probability[i], scale));
        else
            printf("%f\n", probability[i]);
    }

    free(buff);

    return 0;
}

int ecalc(char *, uint8_t);

int main(int argc, char *argv[])
{
    uint8_t flags = 0x00;
    char *filename = NULL;
  
    for (int opt = 0; opt != -1;) {
        opt = getopt(argc, argv, "epogvf:");
        switch(opt) {
        case 'e':
            flags |= E_FLAG;
            break;
        case 'p':
            flags |= P_FLAG;
            break;
        case 'o':
            flags |= O_FLAG;
            break;
        case 'g':
            flags |= G_FLAG;
            break;
        case 'v':
            flags |= V_FLAG;
            break;
        case 'f':
            filename = optarg;
            break;
        }
    }

    errno = 0;

    if (flags == BAD_FLAGS)
        die("Incorrect flags");

    if (!filename)
        die("No file specified");

    return ecalc(filename, flags);
}

int ecalc(char* filename, uint8_t flags)
{
    FILE *f;
    int file_size;

    f = fopen(filename, "rb");
    if (!f)
        return -1;

    if (bad_file(f)) {
        fclose(f);
        
        if (flags & V_FLAG)
            die("Bad file type");
        return -1;
    }

    feenableexcept( FE_INVALID   |
                    FE_DIVBYZERO |
                    FE_OVERFLOW  |
                    FE_UNDERFLOW );

    file_size = get_distribution(f);
    if (file_size == -1)
        return -1;

    compute_probability(file_size);

    if (flags & P_FLAG)
        display_probabilities(70, flags & G_FLAG ? 1 : 0);
    
    if (flags & O_FLAG)
        display_occurrencies();
    
    if (flags & E_FLAG)
           printf("%f\n", entropy());

    fclose(f);

    return 0;
}
