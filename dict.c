#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "dict.h"

struct dict_item {
  // Each word is at most 100 bytes.
  char word[100];
  // The length of each word.
  size_t len;
};

struct dict_t {
  // The path to the underlying file
  char *path;
  // The file descriptor of the opened file. Set by dictionary_open_map.
  int fd;
  // How many items the mmaped file should store (this will determine the size).
  // There are ~500k words in /usr/share/dict/words.
  size_t num_items;
  // A pointer to the first item.
  struct dict_item *base;
};

// Construct a new dict_t struct.
// data_file is where to write the data,
// num_items is how many items this data file should store.
struct dict_t *dictionary_new(char *data_file, size_t num_items) {
  struct dict_t *dict = malloc(sizeof(struct dict_t));
  dict->path = data_file;
  dict->num_items = num_items;
  return dict;
}

// Computes the size of the underlying file based on the # of items and the size
// of a dict_item.
size_t dictionary_len(struct dict_t *dict) {
  return dict->num_items * sizeof(struct dict_item);
}

// This is a private helper function. It should:
// Open the underlying path (dict->path), ftruncate it to the appropriate length
// (dictionary_len), then mmap it.
int dictionary_open_map(struct dict_t *dict) {
  size_t len;
  dict->fd = open(dict->path, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  if (dict->fd == -1) {
    perror("open");
    return EXIT_FAILURE;
  }
  len = dictionary_len(dict);

  if (ftruncate(dict->fd, len) == -1) {
    perror("truncate");
    return EXIT_FAILURE;
  }
  dict->base = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, dict->fd, 0);
  if (dict->base == MAP_FAILED) {
    perror("mmap");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

// Read the file at input. For each line in input, create a new dictionary
// entry.
int dictionary_generate(struct dict_t *dict, char *input) {
  int i;
  i = 0;
  char buffer[100];
  char b[100];
  size_t word_len;
  FILE *fp;
  int open_map_check = dictionary_open_map(dict);
  if (open_map_check == 0) {
    fp = fopen(input, "r");
    if (fp != NULL) {
      while (fgets(buffer, sizeof(buffer), fp)) {
        word_len = strlen(buffer);
        for (int j = 0; j < word_len - 1; j++) {
          b[j] = buffer[j];
        }
        strcpy(dict->base[i].word, b);
        dict->base[i].len = (size_t)strlen(b);
        i = i + 1;
        memset(buffer, 0, sizeof(buffer));
        memset(b, 0, sizeof(b));
      }
    } else {
      return 0;
    }
  } else {
    return 0;
  }
  fclose(fp);
  return 1;
}

// load a dictionary that was generated by calling generate.
int dictionary_load(struct dict_t *dict) {
  if (dictionary_open_map(dict) != 0) {
    return 0;
  } else {
    return 1;
  }
}

// returns pointer to word if it exists, null otherwise
char *dictionary_exists(struct dict_t *dict, char *word) {
  for (int i = 0; i < dict->num_items; i++) {
    char *addr = (char *)(dict->base + i * sizeof(struct dict_item));
    if (!strcmp(dict->base[i].word, word)) {
      return addr;
    }
  }
  return NULL;
}

//// Count of words with len > n
int dictionary_larger_than(struct dict_t *dict, size_t n) {
  int count;
  count = 0;
  if ((n >= 0) && (n < dict->num_items)) {
    for (int i = 0; i < dict->num_items; i++) {
      if (dict->base[i].len > n) {
        count = count + 1;
      } else {
        count = count;
      }
    }
  } else {
    return 0;
  }
  return count;
}

// Count of words with len < n
int dictionary_smaller_than(struct dict_t *dict, size_t n) {
  int count;
  count = 0;
  if ((n >= 0) && (n < dict->num_items)) {
    for (int i = 0; i < dict->num_items; i++) {
      if ((dict->base[i].len < n) && (dict->base[i].len != 0)) {
        count = count + 1;
      } else {
        count = count;
      }
    }
  } else {
    return 0;
  }
  return count;
}

// Count of words with len == n
int dictionary_equal_to(struct dict_t *dict, size_t n) {
  int count;
  count = 0;
  if ((n >= 0) && (n < dict->num_items)) {
    for (int i = 0; i < dict->num_items; i++) {
      if (dict->base[i].len == n) {
        count = count + 1;
      } else {
        count = count;
      }
    }
  } else {
    return 0;
  }
  return count;
}

// Unmaps the given dictionary.
// Free/destroy the underlying dict. Does NOT delete the database file.
void dictionary_close(struct dict_t *dict) {
  msync(dict->base, dict->num_items, MS_SYNC);
  munmap(dict->base, dict->num_items);
  free(dict);
}

// The rest of the functions should be whatever is left from the header that
// hasn't been defined yet.
// Good luck!
