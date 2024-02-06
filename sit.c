#include <ctype.h>
#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#define MAX_FILENAME_LENGTH 1000
#define MAX_COMMIT_MESSAGE_LENGTH 2000
#define MAX_LINE_LENGTH 1000
#define MAX_MESSAGE_LENGTH 1000
#define MAX_PATH 256
#define MAX_PATH_LEN 1024
#define MAX_ALIAS_LENGTH 50
#define debug(x) printf("%s", x);
#define MAX_FILENAME_LENGTH 1000
#define MAX_LINE_LENGTH 1000
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_RESET "\x1b[0m"

void print_command(int argc, char *const argv[]);
int run_init(int argc, char *const argv[]);
int create_configs(int argc, char *const argv[]);
int fileExists(const char *path);
void ensureConfigDirectory();
int run_add(int argc, char *const argv[]);
int sit_add(char *filepath, FILE *stagedOrNotFile);
int sit_reset(char *filepath, FILE *stagedOrNotFile);
int sit_undo_reset(int depth);

int run_reset(int argc, char *const argv[]);
int remove_from_staging(char *filepath);
int run_commit(int argc, char *const argv[]);
// int inc_last_commit_ID();
// bool check_file_directory_exists(char *filepath);
// int commit_staged_file(int commit_ID, char *filepath);
// int track_file(char *filepath);
// bool is_tracked(char *filepath);
// int create_commit_file(int commit_ID, char *message);
// int find_file_last_commit(char *filepath);

// int run_checkout(int argc, char *const argv[]);
// int find_file_last_change_before_commit(char *filepath, int commit_ID);
// int checkout_file(char *filepath, int commit_ID);
int check = 0;
void print_command(int argc, char *const argv[]) {
  for (int i = 0; i < argc; i++) {
    fprintf(stdout, "%s ", argv[i]);
  }
  fprintf(stdout, "\n");
}

int run_init(int argc, char *const argv[]) {
  char cwd[1024];
  if (getcwd(cwd, sizeof(cwd)) == NULL)
    return 1;
  char tmp_cwd[1024];
  if (getcwd(tmp_cwd, sizeof(tmp_cwd)) == NULL)
    return 1;
  bool exists = false;
  struct dirent *entry;
  do {
    // find .neosit
    DIR *dir = opendir(".");
    if (dir == NULL) {
      fprintf(stderr, "Error opening current directory");
      return 1;
    }
    while ((entry = readdir(dir)) != NULL) {
      if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".sit") == 0)
        exists = true;
    }
    closedir(dir);

    // update current working directory
    if (getcwd(tmp_cwd, sizeof(tmp_cwd)) == NULL)
      return 1;

    // change cwd to parent
    if (strcmp(tmp_cwd, "/") != 0) {
      if (chdir("..") != 0)
        return 1;
    }

  } while (!exists && strcmp(tmp_cwd, "/") != 0);

  // return to the initial cwd
  if (chdir(cwd) != 0)
    return 1;

  if (!exists) {
    if (mkdir(".sit", 0755) != 0 || mkdir(".sit/config", 0755) != 0 ||
        mkdir(".sit/commands", 0755) != 0 || mkdir(".sit/add", 0755) != 0 ||
        mkdir(".sit/branches", 0755) != 0)
      return 1;
  } else {
    fprintf(stderr, "sit repository has already initialized\n");
  }
  return 0;
}
void createConfigFile(char *path, char *content) {
  FILE *file = fopen(path, "w"); // Open in write mode
  if (file == NULL) {
    perror("Error opening file");
    exit(EXIT_FAILURE);
  }

  fprintf(file, "%s\n", content);
  fclose(file);
}

void sitConfig(char *key, char *value, int isLocal) {
  char currentPath[1024];
  char configFolderPath[1024];

  // Get the current working directory
  if (getcwd(currentPath, sizeof(currentPath)) == NULL) {
    perror("Error getting current directory");
    exit(EXIT_FAILURE);
  }

  // Construct the path for .sit folder
  snprintf(configFolderPath, sizeof(configFolderPath), "%s/.sit", currentPath);

  // Check if .sit folder exists
  if (access(configFolderPath, F_OK) == -1) {
    fprintf(stdout, "Error: .sit not found. Did you run 'sit init'?\n");
    exit(EXIT_FAILURE);
  }

  // Choose the appropriate config file based on local or global
  char configFile[1024];
  if (isLocal) {
    snprintf(configFile, sizeof(configFile), "%s/config/localconfig.txt",
             configFolderPath);
  } else {
    snprintf(configFile, sizeof(configFile), "%s/config/globalconfig.txt",
             configFolderPath);
  }

  // Check if the config file exists, if not, create it
  if (access(configFile, F_OK) == -1) {
    FILE *file = fopen(configFile, "w");
    if (file == NULL) {
      perror("Error opening file");
      exit(EXIT_FAILURE);
    }
    fclose(file);
  }

  // Open the config file in append mode and write the key-value pair
  FILE *file = fopen(configFile, "a");
  if (file == NULL) {
    perror("Error opening file");
    exit(EXIT_FAILURE);
  }

  fprintf(file, "%s\n", value);
  fclose(file);

  fprintf(stdout, "Configuration updated successfully.\n");
}

void createCommandAlias(char *alias, char *command) {
  if (strstr(alias, "alias.") != NULL) {
    char *aliasPtr = strstr(alias, "alias.");
    if (aliasPtr != NULL) {
      // Extract the alias from "alias."
      char extractedAlias[MAX_ALIAS_LENGTH];
      strncpy(extractedAlias, aliasPtr + 6, MAX_ALIAS_LENGTH - 1);
      extractedAlias[MAX_ALIAS_LENGTH - 1] = '\0'; // Ensure null-terminated

      // Get the current working directory
      char currentPath[1024];
      if (getcwd(currentPath, sizeof(currentPath)) == NULL) {
        perror("Error getting current directory");
        exit(EXIT_FAILURE);
      }

      // Construct the path for .sit/commands folder
      char commandFolderPath[1024];
      snprintf(commandFolderPath, sizeof(commandFolderPath), "%s/.sit/commands",
               currentPath);

      // Check if .sit/commands folder exists
      if (access(commandFolderPath, F_OK) == -1) {
        fprintf(stdout,
                "Error: .sit/commands not found. Did you run 'sit init'?\n");
        exit(EXIT_FAILURE);
      }

      // Choose the appropriate command file
      char commandFile[1024];
      snprintf(commandFile, sizeof(commandFile), "%s/%s.txt", commandFolderPath,
               extractedAlias);

      // Save the original working directory
      char originalPath[1024];
      strcpy(originalPath, currentPath);

      // Check if the command file exists, if not, create it
      if (access(commandFile, F_OK) == -1) {
        FILE *file = fopen(commandFile, "w");
        if (file == NULL) {
          perror("Error opening file");
          exit(EXIT_FAILURE);
        }

        fprintf(file, "%s\n", command);
        fclose(file);

        // Restore the original working directory
        if (chdir(originalPath) != 0) {
          perror("Error changing back to the original directory");
          exit(EXIT_FAILURE);
        }

        fprintf(stdout, "Alias created successfully.\n");
      } else {
        // Restore the original working directory
        if (chdir(originalPath) != 0) {
          perror("Error changing back to the original directory");
          exit(EXIT_FAILURE);
        }

        fprintf(stdout, "Error: Alias '%s' already exists.\n", extractedAlias);
      }
    }
  } else {
    fprintf(stdout, "Invalid alias format: %s\n", alias);
  }
}

int create_configs(int argc, char *const argv[]) {
  if (argc == 5 &&
      (strcmp(argv[2], "-local") != 0 && strcmp(argv[2], "-global") != 0)) {
    fprintf(stdout,
            "Usage: %s config -local/-global user.name \"name\" user.email "
            "\"email\"\n",
            argv[0]);
    return 1;
  }

  int isLocal = (strcmp(argv[2], "-local") == 0) ? 1 : 0;
  char *result;
  if (argc == 4) {
    // Handle alias creation when only alias.arc is provided
    if (strstr(argv[2], "alias.") != NULL) {
      char *alias = argv[2];
      char *command = argv[3];
      createCommandAlias(alias, command);
      return 0;
    }
  }

  // Iterate through the command-line arguments to extract key and value
  for (int i = 3; i < argc; i += 2) {
    char *key = argv[i];
    char *value = argv[i + 1];

    if (strcmp(key, "user.name") == 0 || strcmp(key, "user.email") == 0) {
      sitConfig(key, value, isLocal);
    } else {
      fprintf(stdout, "Invalid key: %s\n", key);
      return 1;
    }
  }
  return 0;
}
int run_command(char *command_name) {
  chdir("/Users/nazaninqaffari/desktop/fop");
  // Construct the path for .sit/commands folder
  char commandFolderPath[1024];
  if (getcwd(commandFolderPath, sizeof(commandFolderPath)) == NULL) {
    perror("Error getting current directory");
    exit(EXIT_FAILURE);
  }
  snprintf(commandFolderPath, sizeof(commandFolderPath), "%s/.sit/commands",
           commandFolderPath);

  // Choose the appropriate command file
  char commandFile[1024];
  snprintf(commandFile, sizeof(commandFile), "%s/%s.txt", commandFolderPath,
           command_name);

  // Check if the command file exists
  if (access(commandFile, F_OK) == -1) {
    fprintf(stdout, "Unknown command: %s\n", command_name);
    return 1;
  }

  // Read the command from the file
  FILE *file = fopen(commandFile, "r");
  if (file == NULL) {
    perror("Error opening command file");
    exit(EXIT_FAILURE);
  }

  char command[MAX_LINE_LENGTH];
  if (fgets(command, sizeof(command), file) != NULL) {
    // Remove the newline character if present
    int length = strlen(command);
    if (length > 0 && command[length - 1] == '\n') {
      command[length - 1] = '\0';
    }

    // Execute the command
    printf("Executing command: %s\n", command);

    // Construct the full path to the sit.exec executable
    char sitExecPath[1024];
    snprintf(sitExecPath, sizeof(sitExecPath), "./sit %s", command);
    // char sitDirectory[1024];
    // snprintf(sitDirectory, sizeof(sitDirectory), "%s/.sit",
    // commandFolderPath);

    // Change the working directory to the .sit directory
    chdir("/Users/nazaninqaffari/desktop/fop");
    char **token = (char **)malloc(100);
    for (int i = 0; i < 100; i++) {
      *(token + i) = (char *)malloc(20);
    }
    int index = 0;

    token[0] = strtok(sitExecPath, " ");
    // printf("%s ", token[0]);
    while (token[index] != NULL) {
      index++;
      token[index] = strtok(NULL, " ");
      // printf("%s ", token[index]);
    }
    if (strcmp(token[1], "init") == 0) {
      return run_init(index, token);
    } else if (strcmp(token[1], "add") == 0) {
      return run_add(index, token);
    } else if (strcmp(token[1], "config") == 0) {
      return create_configs(index, token);
    }
    // else if (strcmp(token[1], "reset") == 0) {
    //   return run_reset(index, token);
    // } else if (strcmp(token[1], "commit") == 0) {
    //   return run_commit(index, token);
    // }
    // else if (strcmp(token[1], "checkout") == 0) {
    //  return run_checkout(index, token);
    //}
    fclose(file);
    return 0;
  }
}
int run_add(int argc, char *const argv[]) {
  char *filepath1 = (char *)malloc(1000);
  if (argc > 3 && strcmp(argv[2], "-n") != 0 && strcmp(argv[2], "-f") != 0) {
    chdir("/Users/nazaninqaffari/desktop/fop");
    for (int i = 2; i < argc; i++) {
      strcpy(filepath1, argv[i]);
      char stagedOrNotPath[MAX_FILENAME_LENGTH];
      snprintf(stagedOrNotPath, sizeof(stagedOrNotPath),
               ".sit/add/stagedOrNot");
      FILE *stagedOrNotFile = fopen(stagedOrNotPath, "a");
      char file_path[MAX_FILENAME_LENGTH];
      realpath(filepath1, file_path);
      sit_add(file_path, stagedOrNotFile);
      fclose(stagedOrNotFile);
    }
    for (int i = 2; i < argc; i++) {
      fprintf(stdout, "File or directory '%s' added to staging.\n", argv[i]);
    }
    return 0;
  }
  char *filepath2 = (char *)malloc(1000);
  if (argc >= 4 && strcmp(argv[2], "-f") == 0) {
    // chdir("/Users/nazaninqaffari/desktop/fop");
    for (int i = 3; i < argc; i++) {
      strcpy(filepath2, argv[i]);
      char stagedOrNotPath[MAX_FILENAME_LENGTH];
      snprintf(stagedOrNotPath, sizeof(stagedOrNotPath),
               ".sit/add/stagedOrNot");
      FILE *stagedOrNotFile = fopen(stagedOrNotPath, "a");
      char file_path[MAX_FILENAME_LENGTH];
      realpath(filepath2, file_path);
      sit_add(file_path, stagedOrNotFile);
      fclose(stagedOrNotFile);
    }
    for (int i = 3; i < argc; i++) {
      fprintf(stdout, "File or directory '%s' added to staging.\n", argv[i]);
    }
    return 0;
  }
  if (argc < 3) {
    fprintf(stdout, "Usage: %s add [file/directory path]\n", argv[0]);
    return 1;
  }
  char *filepath = argv[2];
  // Check if the specified file or directory exists
  struct stat path_stat;
  char file_path[MAX_FILENAME_LENGTH];
  realpath(filepath, file_path);

  if (stat(file_path, &path_stat) != 0) {
    fprintf(stderr, "Error: File or directory '%s' not found.\n", filepath);
    return 1;
  }

  // Check if the stagedOrNot file exists, if not, create it
  char stagedOrNotPath[MAX_FILENAME_LENGTH];
  snprintf(stagedOrNotPath, sizeof(stagedOrNotPath), ".sit/add/stagedOrNot");
  FILE *stagedOrNotFile = fopen(stagedOrNotPath, "a");
  if (stagedOrNotFile == NULL) {
    fprintf(stderr, "Error opening .sit/add/stagedOrNot file.\n");
    return 1;
  }

  // Call sit_add function to process the specified file or directory
  if (sit_add(file_path, stagedOrNotFile) != 0) {
    fclose(stagedOrNotFile);
    return 1;
  }
  // Close the stagedOrNot file
  fclose(stagedOrNotFile);
  fprintf(stdout, "File or directory '%s' added to staging.\n", filepath);
  return 0;
}
// Call sit_add function to process the specified file or directory
int is_directory(const char *path) {
  struct stat path_stat;
  stat(path, &path_stat);
  return S_ISDIR(path_stat.st_mode);
}

int sit_add(char *filepath, FILE *stagedOrNotFile) {
  // Print the filepath to stagedOrNot
  char file_path[MAX_FILENAME_LENGTH];
  fprintf(stagedOrNotFile, "%s\n", filepath);
  // Check if the path is a directory
  if (is_directory(filepath)) {
    // Iterate over files in the directory
    DIR *dir = opendir(filepath);
    if (dir == NULL) {
      fprintf(stderr, "Error opening directory: %s\n", filepath);
      return 1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
      // Skip '.' and '..'
      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
        continue;
      }

      // Concatenate the directory path with the file name
      snprintf(file_path, sizeof(file_path), "%s/%s", filepath, entry->d_name);
      // Recursively call sit_add for each file in the directory
      if (sit_add(file_path, stagedOrNotFile) != 0) {
        closedir(dir);
        return 1;
      }
    }
    closedir(dir);
  } else {
    // Create a copy of the file in the add folder
    char *lastWord;
    lastWord = strrchr(filepath, '/');
    if (lastWord != NULL) {
      lastWord++;
    }
    if (lastWord == NULL) {
      lastWord = filepath;
    }
    char addFilePath[MAX_FILENAME_LENGTH];
    snprintf(addFilePath, sizeof(addFilePath), ".sit/add/%s", lastWord);
    // printf("this is filepath:%s\n", filepath);
    // printf("this is addfilepath:%s\n", filepath);
    // char where[1900];
    // getcwd(where, sizeof(where));
    // printf("this is where: %s", where);
    FILE *srcFile = fopen(filepath, "r");
    FILE *destFile = fopen(addFilePath, "w");

    if (srcFile == NULL || destFile == NULL) {
      fprintf(stderr, "Error copying file to .sit/add folder.\n");
      return 1;
    }

    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), srcFile) != NULL) {
      fprintf(destFile, "%s", line);
    }

    fclose(srcFile);
    fclose(destFile);
    // printf("File '%s' added to staging.\n", filepath);
  }
  return 0;
}
int run_reset(int argc, char *const argv[]) {
  if (argc < 3) {
    fprintf(stdout, "Usage: %s reset [file/directory path]\n", argv[0]);
    return 1;
  }
  if (strcmp(argv[2], "-undo") == 0) {
    if (argc != 4) {
      fprintf(stdout, "Usage: %s reset -undo [depth]\n", argv[0]);
      return 1;
    }

    int depth = atoi(argv[3]);
    if (depth <= 0) {
      fprintf(stdout, "Invalid depth. Please provide a positive integer.\n");
      return 1;
    }
    return sit_undo_reset(depth);
  }

  char *filepath = argv[2];
  // Check if the specified file or directory exists
  struct stat path_stat;
  char file_path[MAX_FILENAME_LENGTH];
  realpath(filepath, file_path);
  // printf("%s ", file_path);

  if (stat(file_path, &path_stat) != 0) {
    fprintf(stderr, "Error: File or directory '%s' not found.\n", filepath);
    return 1;
  }

  // Check if the .sit folder exists
  char sitFolderPath[MAX_FILENAME_LENGTH];
  snprintf(sitFolderPath, sizeof(sitFolderPath), ".sit");
  if (access(sitFolderPath, F_OK) == -1) {
    fprintf(stderr, "Error: .sit not found. Did you run 'sit init'?\n");
    return 1;
  }

  // Check if the .sit/add folder exists
  char addFolderPath[MAX_FILENAME_LENGTH];
  snprintf(addFolderPath, sizeof(addFolderPath), ".sit/add");
  if (access(addFolderPath, F_OK) == -1) {
    fprintf(stderr, "Error: .sit/add not found. No files staged for reset.\n");
    return 1;
  }

  // Check if the .sit/add/stagedOrNot file exists
  char stagedOrNotPath[MAX_FILENAME_LENGTH];
  snprintf(stagedOrNotPath, sizeof(stagedOrNotPath), ".sit/add/stagedOrNot");
  FILE *stagedOrNotFile = fopen(stagedOrNotPath, "r");
  if (stagedOrNotFile == NULL) {
    fprintf(stderr, "Error opening .sit/add/stagedOrNot file.\n");
    return 1;
  }

  // Call sit_reset function to process the specified file or directory
  if (sit_reset(file_path, stagedOrNotFile) != 0) {
    fclose(stagedOrNotFile);
    return 1;
  }

  // Close the stagedOrNot file
  fclose(stagedOrNotFile);
  fprintf(stdout, "File or directory '%s' reset successfully.\n", filepath);
  return 0;
}

int sit_reset(char *filepath, FILE *stagedOrNotFile) {

  char stagedOrNotPath[MAX_FILENAME_LENGTH];
  snprintf(stagedOrNotPath, sizeof(stagedOrNotPath), ".sit/add/stagedOrNot");

  // Check if the path is a directory
  if (is_directory(filepath)) {
    // Iterate over files in the directory
    DIR *dir = opendir(filepath);
    if (dir == NULL) {
      fprintf(stderr, "Error opening directory: %s\n", filepath);
      return 1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
      // Skip '.' and '..'
      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
        continue;
      }

      // Concatenate the directory path with the file name
      char file_path[MAX_FILENAME_LENGTH];
      snprintf(file_path, sizeof(file_path), "%s/%s", filepath, entry->d_name);
      // Recursively call sit_reset for each file in the directory
      if (sit_reset(file_path, stagedOrNotFile) != 0) {
        closedir(dir);
        return 1;
      }
    }
    closedir(dir);
  } else {
    // printf("this is the file path : %s", filepath);
    // Check if the file is staged
    FILE *tmpFile = tmpfile();
    char line[MAX_FILENAME_LENGTH];
    int found = 0;
    stagedOrNotFile = fopen(stagedOrNotPath, "r");
    FILE *resetfiles = fopen(".sit/add/resetfiles", "a");
    while (fgets(line, sizeof(line), stagedOrNotFile) != NULL) {
      // Remove the newline character if present
      int length = strlen(line);
      if (length > 0 && line[length - 1] == '\n') {
        line[length - 1] = '\0';
      }

      // Check if the file path matches
      if (strstr(line, filepath) != NULL) {
        found = 1;
        fprintf(resetfiles, "%s\n", line);
        continue; // Skip this line and the next 'staged' line
      }

      // Write the line to the temporary file
      fprintf(tmpFile, "%s\n", line);
    }

    // Close the original stagedOrNot file
    fclose(stagedOrNotFile);

    // If the file is not staged, return with success
    if (!found) {
      fprintf(stdout, "This file has not been added in the past.");
      fclose(tmpFile);
      return 0;
    }

    // Remove the temporary file and rename it to the original stagedOrNot file
    remove(stagedOrNotPath);
    rewind(tmpFile);
    stagedOrNotFile = fopen(stagedOrNotPath, "w");
    while (fgets(line, sizeof(line), tmpFile) != NULL) {
      fprintf(stagedOrNotFile, "%s", line);
    }
    fclose(tmpFile);
    fclose(stagedOrNotFile);

    // Remove the file from the .sit/add folder
    char *lastWord = strrchr(filepath, '/');
    if (lastWord != NULL) {
      lastWord++;
    }
    if (lastWord == NULL) {
      lastWord = filepath;
    }
    char addFilePath[MAX_FILENAME_LENGTH];
    snprintf(addFilePath, sizeof(addFilePath), ".sit/add/%s", lastWord);
    remove(addFilePath);
  }
  return 0;
}
int sit_undo_reset(int depth) {
  // Check if the .sit/add/resetfiles file exists
  char resetFilesPath[MAX_FILENAME_LENGTH];
  snprintf(resetFilesPath, sizeof(resetFilesPath), ".sit/add/resetfiles");
  FILE *resetFilesFile = fopen(resetFilesPath, "r");
  if (resetFilesFile == NULL) {
    fprintf(stderr, "Error opening .sit/add/resetfiles file.\n");
    return 1;
  }
  // Find the last line
  char line[MAX_FILENAME_LENGTH];
  fseek(resetFilesFile, 0, SEEK_END);

  for (int i = 0; i < depth; ++i) {
    resetFilesFile = fopen(resetFilesPath, "r");
    long position = ftell(resetFilesFile);
    while (position > 0) {
      fseek(resetFilesFile, --position, SEEK_SET);

      if (fgetc(resetFilesFile) == '\n') {
        // Move past the newline character
        // fseek(resetFilesFile, position + 1, SEEK_SET);

        // Read the last line
        if (fgets(line, sizeof(line), resetFilesFile) == NULL) {
          fprintf(stderr, "Error reading resetfiles file.\n");
          fclose(resetFilesFile);
          return 1;
        }
        break;
      }
    }
    // Handle the case where the file consists of a single line without a
    // newline
    if (position == 0) {
      // fseek(resetFilesFile, 0, SEEK_SET);

      if (fgets(line, sizeof(line), resetFilesFile) == NULL) {
        fprintf(stderr, "Error reading resetfiles file.\n");
        fclose(resetFilesFile);
        return 1;
      }
    }
    int length1 = strlen(line);
    if (length1 > 0 && line[length1 - 1] == '\n') {
      line[length1 - 1] = '\0';
    }

    // Add the line to stagedOrNot
    FILE *stagedOrNotFile = fopen(".sit/add/stagedOrNot", "a");
    fprintf(stagedOrNotFile, "%s", line);
    fclose(stagedOrNotFile);

    // Copy the file to the add folder
    // Copy the file to the add folder
    char *lastWord = strrchr(line, '/');
    if (lastWord != NULL) {
      lastWord++;
    }
    if (lastWord == NULL) {
      lastWord = line;
    }

    char addFilePath[MAX_FILENAME_LENGTH];
    snprintf(addFilePath, sizeof(addFilePath), ".sit/add/%s", lastWord);
    chdir("/Users/nazaninqaffari/Desktop/fop");
    FILE *srcFile = fopen(line, "r");
    FILE *destFile = fopen(addFilePath, "w");

    if (srcFile == NULL || destFile == NULL) {
      fprintf(stderr, "Error copying file to .sit/add folder.\n");
      return 1;
    }

    char line2[MAX_LINE_LENGTH];
    while (fgets(line2, sizeof(line2), srcFile) != NULL) {
      fprintf(destFile, "%s", line2);
    }

    fclose(srcFile);
    fclose(destFile);
    // printf("File '%s' added to staging.\n", filepath);
    // Close the resetfiles file
    fclose(resetFilesFile);

    // Remove the last 'depth' lines from resetfiles
    char line1[MAX_FILENAME_LENGTH];
    FILE *tmpFile = tmpfile();
    resetFilesFile = fopen(resetFilesPath, "r");
    while (fgets(line1, sizeof(line1), resetFilesFile) != NULL) {
      // Remove the newline character if present
      int length = strlen(line1);
      if (length > 0 && line1[length - 1] == '\n') {
        line1[length - 1] = '\0';
      }
      if (strstr(line1, line) != NULL) {
        continue;
      }
      fprintf(tmpFile, "%s\n", line1);
    }

    // Close the original resetfiles file
    fclose(resetFilesFile);

    // Remove the original resetfiles file
    remove(resetFilesPath);

    // Rename the temporary file to the original resetfiles file
    rewind(tmpFile);
    resetFilesFile = fopen(resetFilesPath, "w");
    while (fgets(line1, sizeof(line1), tmpFile) != NULL) {
      fprintf(resetFilesFile, "%s\n", line1);
    }

    // Close the resetfiles file
    fclose(tmpFile);
    fclose(resetFilesFile);
  }
  return 0;
}
int read_commit_number() {
  FILE *commit_number_file = fopen(".sit/commit_number.txt", "r");
  if (commit_number_file == NULL) {
    // File doesn't exist, create it with initial value 1
    FILE *new_file = fopen(".sit/commit_number.txt", "w");
    if (new_file != NULL) {
      fprintf(new_file, "1");
      fclose(new_file);
      return 1;
    } else {
      fprintf(stderr, "Error creating commit_number.txt file.\n");
      exit(EXIT_FAILURE);
    }
  }

  int commit_number;
  fscanf(commit_number_file, "%d", &commit_number);
  fclose(commit_number_file);
  return commit_number;
}

void write_commit_number(int commit_number) {
  FILE *commit_number_file = fopen(".sit/commit_number.txt", "w");
  if (commit_number_file != NULL) {
    fprintf(commit_number_file, "%d", commit_number);
    fclose(commit_number_file);
  } else {
    fprintf(stderr, "Error writing to commit_number.txt file.\n");
    exit(EXIT_FAILURE);
  }
}

void update_file_info(const char *commit_folder, const int commit_id,
                      int num_files, const char *branch, const char *author,
                      const char *commit_message) {
  // Get the current time and format it
  time_t t;
  struct tm *info;
  time(&t);
  info = localtime(&t);
  char time_str[20];
  strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", info);
  FILE *info_file;
  FILE *commit_log_file; // New file to log commit information
  FILE *temp_file;       // Temporary file for updating commit log file
  FILE *time_log_file;
  FILE *temp_file1;
  FILE *message_file;
  FILE *temp_file2;
  char info_file_path[100];
  char commit_log_file_path[100];
  char temp_file_path[100];
  char time_log_file_path[100];
  char temp_file1_path[100];
  char message_log_file_path[100];
  char temp_file2_path[100];

  // Construct the path for the info file inside the commit folder
  sprintf(info_file_path, "%s/info.txt", commit_folder);

  // Construct the path for the commit log file inside the .sit folder
  sprintf(commit_log_file_path, ".sit/commit_log.txt");
  sprintf(time_log_file_path, ".sit/timeCapture.txt");
  sprintf(message_log_file_path, ".sit/Messages.txt");
  // Construct the path for the temporary file
  sprintf(temp_file_path, ".sit/temp_commit_log.txt");
  sprintf(temp_file1_path, ".sit/temp_File1.txt");
  sprintf(temp_file2_path, ".sit/temp_File2.txt");
  // Open the info file in append mode
  info_file = fopen(info_file_path, "a");

  if (info_file == NULL) {
    fprintf(stderr, "Error opening info file.\n");
    exit(EXIT_FAILURE);
  }

  // Open the commit log file in read mode
  time_log_file = fopen(time_log_file_path, "r");
  commit_log_file = fopen(commit_log_file_path, "r");
  message_file = fopen(message_log_file_path, "r");
  if (commit_log_file == NULL || time_log_file == NULL ||
      message_file == NULL) {
    // If the commit log file doesn't exist, create it
    time_log_file = fopen(time_log_file_path, "w");
    commit_log_file = fopen(commit_log_file_path, "w");
    message_file = fopen(message_log_file_path, "w");
    if (commit_log_file == NULL || message_file == NULL ||
        time_log_file == NULL) {
      fprintf(stderr, "Error creating commit log file.\n");
      fclose(info_file);
      exit(EXIT_FAILURE);
    }
    // Write commit information to the commit log file
    fprintf(message_file, "%s\n", commit_message);
    fprintf(time_log_file, "%s\n", time_str);
    fprintf(commit_log_file, "Commit ID: %d\n", commit_id);
    fprintf(commit_log_file, "Number of Files Committed: %d\n", num_files);
    fprintf(commit_log_file, "Branch: %s\n", branch);
    fprintf(commit_log_file, "Author: %s\n", author);
    fprintf(commit_log_file, "Date and Time: %s\n", time_str);
    fprintf(commit_log_file, "Commit Message: %s\n", commit_message);
  } else {
    // Open the temporary file in write mode
    temp_file = fopen(temp_file_path, "w");
    temp_file1 = fopen(temp_file1_path, "w");
    temp_file2 = fopen(temp_file2_path, "w");
    if (temp_file == NULL) {
      fprintf(stderr, "Error opening temporary file.\n");
      fclose(info_file);
      fclose(commit_log_file);
      exit(EXIT_FAILURE);
    }
    fprintf(temp_file2, "%s\n", commit_message);
    fprintf(temp_file1, "%s\n", time_str);
    // Write new commit information to the temporary file
    fprintf(temp_file, "Commit ID: %d\n", commit_id);
    fprintf(temp_file, "Number of Files Committed: %d\n", num_files);
    fprintf(temp_file, "Branch: %s\n", branch);
    fprintf(temp_file, "Author: %s\n", author);
    fprintf(temp_file, "Date and Time: %s\n", time_str);
    fprintf(temp_file, "Commit Message: %s\n", commit_message);

    // Copy the existing content from the commit log file to the temporary file
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), commit_log_file) != NULL) {
      fprintf(temp_file, "%s", buffer);
    }
    char buffer1[1024];
    while (fgets(buffer1, sizeof(buffer1), time_log_file) != NULL) {
      fprintf(temp_file1, "%s", buffer1);
    }
    char buffer2[1024];
    while (fgets(buffer2, sizeof(buffer2), message_file) != NULL) {
      fprintf(temp_file2, "%s", buffer2);
    }

    // Close both files
    fclose(commit_log_file);
    fclose(temp_file);
    fclose(time_log_file);
    fclose(message_file);
    fclose(temp_file1);
    fclose(temp_file2);
    // Rename the temporary file to the original commit log file
    if (rename(temp_file_path, commit_log_file_path) != 0) {
      fprintf(stderr, "Error renaming files.\n");
      fclose(info_file);
      exit(EXIT_FAILURE);
    }
    if (rename(temp_file1_path, time_log_file_path) != 0) {
      fprintf(stderr, "Error renaming files.\n");
      fclose(info_file);
      exit(EXIT_FAILURE);
    }
    if (rename(temp_file2_path, message_log_file_path) != 0) {
      fprintf(stderr, "Error renaming files.\n");
      fclose(info_file);
      exit(EXIT_FAILURE);
    }
    commit_log_file = fopen(commit_log_file_path, "a");
    time_log_file = fopen(time_log_file_path, "a");
    message_file = fopen(message_log_file_path, "a");
  }
  fprintf(info_file, "Commit ID: %d\n", commit_id);
  fprintf(info_file, "Number of Files Committed: %d\n", num_files);
  fprintf(info_file, "Branch: %s\n", branch);
  fprintf(info_file, "Author: %s\n", author);
  fprintf(info_file, "Date and Time: %s\n", time_str);
  fprintf(info_file, "Commit Message: %s\n", commit_message);
  // Print commit information for the user
  printf("Commit ID: %d\n", commit_id);
  printf("Number of Files Committed: %d\n", num_files);
  printf("Branch: %s\n", branch);
  printf("Author: Nazi\n");
  printf("Date and Time: %s\n", time_str);
  printf("Commit Message: %s\n", commit_message);

  // Close both files
  fclose(info_file);
  fclose(commit_log_file);
}

void set_shortcut(const char *shortcut_name, const char *message) {
  // Implement the logic to set a new shortcut
  // You can store the mapping in a file or any suitable data structure
  // For example, you can create a file named ".sit/shortcuts.txt"
  // and store the mappings in the format "shortcut_name=message"

  FILE *shortcut_file = fopen(".sit/shortcuts.txt", "a");
  if (shortcut_file == NULL) {
    fprintf(stderr, "Error opening shortcuts file.\n");
    exit(EXIT_FAILURE);
  }

  fprintf(shortcut_file, "%s=%s\n", shortcut_name, message);
  fprintf(stdout, "shortcut added for %s successfully", shortcut_name);
  fclose(shortcut_file);
}

void replace_shortcut(const char *shortcut_name, const char *new_message) {
  // Implement the logic to replace an existing shortcut
  // You may need to read the existing shortcuts from a file or data structure,
  // update the desired one, and write the changes back

  FILE *shortcut_file = fopen(".sit/shortcuts.txt", "r");
  if (shortcut_file == NULL) {
    fprintf(stderr, "Error opening shortcuts file.\n");
    exit(EXIT_FAILURE);
  }

  FILE *temp_file = fopen(".sit/temp_shortcuts.txt", "w");
  if (temp_file == NULL) {
    fprintf(stderr, "Error creating temp file.\n");
    fclose(shortcut_file);
    exit(EXIT_FAILURE);
  }

  char line[256];
  int shortcut_found = 0;

  while (fgets(line, sizeof(line), shortcut_file) != NULL) {
    char current_shortcut[50];
    char current_message[200];

    if (sscanf(line, "%49[^=]=%199[^\n]", current_shortcut, current_message) ==
        2) {
      if (strcmp(current_shortcut, shortcut_name) == 0) {
        fprintf(temp_file, "%s=%s\n", shortcut_name, new_message);
        shortcut_found = 1;
      } else {
        fprintf(temp_file, "%s=%s\n", current_shortcut, current_message);
      }
    }
  }

  fclose(shortcut_file);
  fclose(temp_file);

  remove(".sit/shortcuts.txt");
  rename(".sit/temp_shortcuts.txt", ".sit/shortcuts.txt");

  if (!shortcut_found) {
    fprintf(stderr, "Error: Shortcut '%s' not found.\n", shortcut_name);
    exit(EXIT_FAILURE);
  }
  fprintf(stdout, "shortcut replaced for %s successfully", shortcut_name);
}

void remove_shortcut(const char *shortcut_name) {
  // Implement the logic to remove an existing shortcut
  // You may need to read the existing shortcuts from a file or data structure,
  // remove the desired one, and write the changes back

  FILE *shortcut_file = fopen(".sit/shortcuts.txt", "r");
  if (shortcut_file == NULL) {
    fprintf(stderr, "Error opening shortcuts file.\n");
    exit(EXIT_FAILURE);
  }

  FILE *temp_file = fopen(".sit/temp_shortcuts.txt", "w");
  if (temp_file == NULL) {
    fprintf(stderr, "Error creating temp file.\n");
    fclose(shortcut_file);
    exit(EXIT_FAILURE);
  }

  char line[256];
  int shortcut_found = 0;

  while (fgets(line, sizeof(line), shortcut_file) != NULL) {
    char current_shortcut[50];

    if (sscanf(line, "%49[^=]", current_shortcut) == 1) {
      if (strcmp(current_shortcut, shortcut_name) != 0) {
        fprintf(temp_file, "%s", line);
      } else {
        shortcut_found = 1;
      }
    }
  }

  fclose(shortcut_file);
  fclose(temp_file);

  remove(".sit/shortcuts.txt");
  rename(".sit/temp_shortcuts.txt", ".sit/shortcuts.txt");

  if (!shortcut_found) {
    fprintf(stderr, "Error: Shortcut '%s' not found.\n", shortcut_name);
    exit(EXIT_FAILURE);
  }
  fprintf(stdout, "shortcut removed for %s successfully", shortcut_name);
}

int run_command_with_shortcut(int argc, char *const argv[]) {
  if (argc < 6 || strcmp(argv[2], "-m") != 0 || strcmp(argv[4], "-s") != 0) {
    fprintf(stdout, "Usage: %s set -m \"message\" -s shortcut_name\n", argv[0]);
    return 1;
  }

  const char *message = argv[3];
  const char *shortcut_name = argv[5];

  set_shortcut(shortcut_name, message);

  // Now you can use the shortcut in the commit command
  // For example: ./sit commit -s shortcut_name

  return 0;
}

int run_replace_command(int argc, char *const argv[]) {
  if (argc < 6 || strcmp(argv[2], "-m") != 0 || strcmp(argv[4], "-s") != 0) {
    fprintf(stdout, "Usage: %s replace -m \"new_message\" -s shortcut_name\n",
            argv[0]);
    return 1;
  }

  const char *new_message = argv[3];
  const char *shortcut_name = argv[5];

  replace_shortcut(shortcut_name, new_message);

  return 0;
}

int run_remove_command(int argc, char *const argv[]) {
  if (argc < 4 || strcmp(argv[2], "-s") != 0) {
    fprintf(stdout, "Usage: %s remove -s shortcut_name\n", argv[0]);
    return 1;
  }

  const char *shortcut_name = argv[3];

  remove_shortcut(shortcut_name);

  return 0;
}
char *get_message_from_shortcut(const char *shortcut_name) {
  // Implement the logic to retrieve the message based on the shortcut name
  // You can read from the shortcuts file or use any suitable data structure
  // For example, you can create a function similar to set_shortcut to read
  // shortcuts

  FILE *shortcut_file = fopen(".sit/shortcuts.txt", "r");
  if (shortcut_file == NULL) {
    fprintf(stderr, "Error opening shortcuts file.\n");
    exit(EXIT_FAILURE);
  }

  char line[256];
  while (fgets(line, sizeof(line), shortcut_file) != NULL) {
    char current_shortcut[50];
    char current_message[200];

    if (sscanf(line, "%49[^=]=%199[^\n]", current_shortcut, current_message) ==
        2) {
      if (strcmp(current_shortcut, shortcut_name) == 0) {
        fclose(shortcut_file);
        return strdup(current_message); // Return a dynamically allocated copy
                                        // of the message
      }
    }
  }

  fclose(shortcut_file);
  return NULL; // Shortcut not found
}
void update_latest_commit(int new_commit_number) {
  FILE *latest_commit_file = fopen(".sit/latest_commit.txt", "w");
  if (latest_commit_file == NULL) {
    fprintf(stderr, "Error opening latest_commit.txt for writing.\n");
    exit(EXIT_FAILURE);
  }
  fprintf(latest_commit_file, "%d", new_commit_number);
  fclose(latest_commit_file);
}

int read_latest_commit() {
  FILE *latest_commit_file = fopen(".sit/latest_commit.txt", "r");
  if (latest_commit_file == NULL) {
    latest_commit_file = fopen(".sit/latest_commit.txt", "w");
    fprintf(latest_commit_file, "1");
    int latest_commit;
    fscanf(latest_commit_file, "%d", &latest_commit);
    fclose(latest_commit_file);
    return latest_commit;
  }
  int latest_commit;
  fscanf(latest_commit_file, "%d", &latest_commit);
  fclose(latest_commit_file);
  return latest_commit;
}
int run_commit(int argc, char *const argv[]) {
  if (strcmp(argv[2], "-s") == 0) {
    char message[100] = "";       // Assuming MAX_MESSAGE_LENGTH is defined
    char shortcut_name[100] = ""; // Assuming MAX_SHORTCUT_LENGTH is defined
    char branch_name[100] = "";   // Assuming MAX_BRANCH_LENGTH is defined
    for (int i = 2; i < argc - 1; i++) {
      if (strcmp(argv[i], "-s") == 0) {
        strncpy(shortcut_name, argv[i + 1], sizeof(shortcut_name) - 1);
        shortcut_name[sizeof(shortcut_name) - 1] =
            '\0'; // Ensure null-terminated
      } else if (strcmp(argv[i], "-m") == 0) {
        strncpy(message, argv[i + 1], sizeof(message) - 1);
        message[sizeof(message) - 1] = '\0'; // Ensure null-terminated
      } else if (strcmp(argv[i], "-b") == 0) {
        strncpy(branch_name, argv[i + 1], sizeof(branch_name) - 1);
        branch_name[sizeof(branch_name) - 1] = '\0'; // Ensure null-terminated
      }
    }

    if (strcmp(branch_name, "") != 0) {
      // If a shortcut is provided, retrieve the message associated with it
      char *shortcut_message = get_message_from_shortcut(shortcut_name);
      if (shortcut_message == NULL) {
        fprintf(stderr, "Error: Shortcut '%s' not found.\n", shortcut_name);
        return 1;
      }
      // Update message with shortcut message
      strncpy(message, shortcut_message, sizeof(message) - 1);
      message[sizeof(message) - 1] = '\0'; // Ensure null-terminated
    } else if (strcmp(message, "") == 0) {
      fprintf(stdout, "Usage: %s commit -m \"message\" -b branch_name\n",
              argv[0]);
      return 1;
    }
    // Now you can use the extracted or provided message in the commit command
    // For example: ./sit commit -m "message" -b branch_name
    const char *commit_argv[] = {argv[0], "commit", "-m",
                                 message, "-b",     branch_name};
    return run_commit(6, (char *const *)commit_argv);
  }
  int commit_number = read_commit_number();
  int latest_commit_number = read_latest_commit();
  if (commit_number != latest_commit_number) {
    fprintf(stderr, "Error: Please checkout to the latest commit first.\n");
    return 1;
  }
  if (argc < 6 || strcmp(argv[2], "-m") != 0 || strcmp(argv[4], "-b") != 0) {
    fprintf(stdout, "Usage: %s commit -m \"commit message\" -b branch_name\n",
            argv[0]);
    return 1;
  }

  // Extract branch name from the command line arguments
  char *branch_name = argv[5];

  // Check if the .sit folder exists
  char sit_folder_path[MAX_FILENAME_LENGTH];
  snprintf(sit_folder_path, sizeof(sit_folder_path), ".sit");
  if (access(sit_folder_path, F_OK) == -1) {
    fprintf(stderr, "Error: .sit not found. Did you run 'sit init'?\n");
    return 1;
  }

  // Check if the .sit/add folder exists
  char add_folder_path[MAX_FILENAME_LENGTH];
  snprintf(add_folder_path, sizeof(add_folder_path), ".sit/add");
  if (access(add_folder_path, F_OK) == -1) {
    fprintf(stderr, "Error: .sit/add not found. No files staged for commit.\n");
    return 1;
  }

  // Check if there are any files staged for commit
  char stagedOrNotPath[MAX_FILENAME_LENGTH];
  snprintf(stagedOrNotPath, sizeof(stagedOrNotPath), ".sit/add/stagedOrNot");
  FILE *stagedOrNotFile = fopen(stagedOrNotPath, "r");
  if (stagedOrNotFile == NULL) {
    fprintf(stderr, "Error opening .sit/add/stagedOrNot file.\n");
    return 1;
  }
  // Check if the branch folder exists
  char branch_folder_path[MAX_FILENAME_LENGTH];
  snprintf(branch_folder_path, sizeof(branch_folder_path), ".sit/branches/%s",
           branch_name);
  if (access(branch_folder_path, F_OK) == -1) {
    if (mkdir(branch_folder_path, 0755) != 0) {
      fprintf(stderr, "Error creating branch folder.\n");
      fclose(stagedOrNotFile);
      return 1;
    }
  }

  // Create the commit folder in the specified branch
  char commit_folder_path[MAX_FILENAME_LENGTH];
  snprintf(commit_folder_path, sizeof(commit_folder_path),
           ".sit/branches/%s/%d", branch_name, commit_number);
  if (mkdir(commit_folder_path, 0755) != 0) {
    fprintf(stderr, "Error creating commit folder.\n");
    fclose(stagedOrNotFile);
    return 1;
  }

  struct dirent *de;
  DIR *addDir = opendir(add_folder_path);
  if (addDir == NULL) {
    fprintf(stderr, "Error opening .sit/add directory.\n");
    fclose(stagedOrNotFile);
    return 1;
  }
  int numberoffiles = 0;
  while ((de = readdir(addDir)) != NULL) {
    if (strcmp(de->d_name, ".") != 0 && strcmp(de->d_name, "..") != 0) {
      char source_file_path[MAX_FILENAME_LENGTH];
      snprintf(source_file_path, sizeof(source_file_path), "%s/%s",
               add_folder_path, de->d_name);

      char dest_file_path[MAX_FILENAME_LENGTH];
      snprintf(dest_file_path, sizeof(dest_file_path), "%s/%s",
               commit_folder_path, de->d_name);

      // Move the file to the commit folder
      numberoffiles++;
      rename(source_file_path, dest_file_path);
    }
  }
  closedir(addDir);

  // Remove stagedOrNot and resetfiles in .sit/add
  remove(stagedOrNotPath);

  // Create empty stagedOrNot and resetfiles in .sit/add
  FILE *newStagedOrNotFile = fopen(stagedOrNotPath, "w");
  if (newStagedOrNotFile == NULL) {
    fprintf(stderr, "Error creating new .sit/add/stagedOrNot file.\n");
    return 1;
  }
  fclose(newStagedOrNotFile);
  printf("Commit was successful!\n");
  update_file_info(commit_folder_path, commit_number, numberoffiles,
                   branch_name, "Nazi", argv[3]);
  write_commit_number(commit_number + 1);
  update_latest_commit(latest_commit_number + 1);
  // Increment the commit number for the next commit
  return 0;
}
void print_commit_log(const char *commit_log_path, int lines_to_print) {
  FILE *commit_log_file = fopen(commit_log_path, "r");
  if (commit_log_file == NULL) {
    fprintf(stderr, "Error opening commit log file.\n");
    exit(EXIT_FAILURE);
  }

  char buffer[1024];
  int lines_printed = 0;

  while (fgets(buffer, sizeof(buffer), commit_log_file) != NULL) {
    printf("%s", buffer);
    lines_printed++;

    if (lines_to_print > 0 && lines_printed >= lines_to_print) {
      break;
    }
  }

  fclose(commit_log_file);
}

void run_log(int argc, char *argv[]) {
  if (argc == 2) {
    // ./sit log (prints all lines in commit_log.txt)
    print_commit_log(".sit/commit_log.txt", 0);
  } else if (argc == 4 && strcmp(argv[2], "-n") == 0) {
    // ./sit log -n [number] (prints [number]*6 lines from commit_log.txt)
    int lines_to_print = atoi(argv[3]) * 6;
    print_commit_log(".sit/commit_log.txt", lines_to_print);
  } else if (argc == 4 && strcmp(argv[2], "-branch") == 0) {
    const char *branch_name = argv[3];
    char branch_folder[100];
    snprintf(branch_folder, sizeof(branch_folder), ".sit/branches/%s",
             branch_name);

    // Check if the branch folder exists
    if (access(branch_folder, F_OK) == 0) {
      // Open the branch directory
      DIR *dir = opendir(branch_folder);
      if (dir == NULL) {
        fprintf(stderr, "Error opening branch directory '%s'.\n",
                branch_folder);
        exit(EXIT_FAILURE);
      }

      struct dirent *entry;

      // Print commit information for each commit folder in the branch
      while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
          const char *commit_folder_name = entry->d_name;
          if (strcmp(commit_folder_name, ".") != 0 &&
              strcmp(commit_folder_name, "..") != 0) {
            // Create the commit folder path
            char commit_folder[100];
            snprintf(commit_folder, sizeof(commit_folder), "%s/%s",
                     branch_folder, commit_folder_name);

            // Open and print info.txt from each commit folder
            char info_file_path[100];
            snprintf(info_file_path, sizeof(info_file_path), "%s/info.txt",
                     commit_folder);
            FILE *info_file = fopen(info_file_path, "r");

            if (info_file == NULL) {
              fprintf(stderr,
                      "Error opening info.txt file in commit folder %s.\n",
                      commit_folder_name);
              exit(EXIT_FAILURE);
            }

            // Read and print the content of info.txt
            char buffer[1024];
            while (fgets(buffer, sizeof(buffer), info_file) != NULL) {
              printf("%s", buffer);
            }

            fclose(info_file);
          }
        }
      }

      closedir(dir);
    } else {
      fprintf(stderr, "Error: Branch '%s' not found.\n", branch_name);
      exit(EXIT_FAILURE);
    }
  } else if (argc == 4 && strcmp(argv[2], "-author") == 0) {
    print_commit_log(".sit/commit_log.txt", 0);
  } else {
    fprintf(stdout, "Invalid usage of 'log' command.\n");
    exit(EXIT_FAILURE);
  }
}
void filter_log_by_time_range(const char *log_path, const char *time_option,
                              const char *user_time) {
  FILE *log_file = fopen(log_path, "r");
  if (log_file == NULL) {
    fprintf(stderr, "Error opening log file.\n");
    exit(EXIT_FAILURE);
  }

  FILE *time_capture_file = fopen(".sit/timeCapture.txt", "r");
  if (time_capture_file == NULL) {
    fprintf(stderr, "Error opening timeCapture file.\n");
    fclose(log_file);
    exit(EXIT_FAILURE);
  }

  int lines_to_print = 0;
  int less_line_num = 0;
  int found_time = 0;
  int countlines = 0;
  char time_buffer[20];
  while (fgets(time_buffer, sizeof(time_buffer), time_capture_file) != NULL) {
    countlines++;
  }
  rewind(time_capture_file);
  while (fgets(time_buffer, sizeof(time_buffer), time_capture_file) != NULL) {
    // Remove newline character from time_buffer
    time_buffer[strcspn(time_buffer, "\n")] = 0;

    // Increment the count of lines
    lines_to_print++;

    // Compare the time based on the user's specified option
    int time_comparison = strcmp(time_buffer, user_time);
    if ((strcmp(time_option, "-since") == 0 && time_comparison <= 0) ||
        (strcmp(time_option, "-before") == 0 && time_comparison <= 0)) {
      found_time = 1;
      less_line_num = lines_to_print;
    }
    if (found_time) {
      break;
    }
  }
  fclose(time_capture_file);

  // Print the requested lines from the commit log or the message if no match is
  // found
  rewind(log_file); // Reset the log file position to the beginning

  // Skip lines before the desired ones
  if (strcmp(time_option, "-since") == 0) {
    for (int i = 0; i < 6 * (less_line_num + 1); i++) {
      char buffer[1024];
      if (fgets(buffer, sizeof(buffer), log_file) != NULL) {
        printf("%s", buffer);
      }
    }
  }
  if (strcmp(time_option, "-before") == 0) {
    char buffer[1024];

    // Skip lines before the desired ones
    for (int i = 0; i < 6 * (less_line_num - 1); i++) {
      if (fgets(buffer, sizeof(buffer), log_file) == NULL) {
        break; // Break if end of file is reached
      }
    }

    // Print the lines after the skipped ones
    while (fgets(buffer, sizeof(buffer), log_file) != NULL) {
      printf("%s", buffer);
    }
  }

  fclose(log_file);

  // Print the "No matching records found" message if necessary
  if (!found_time) {
    fprintf(stdout, "No matching records found based on the specified time.\n");
  }
}
void search_commits_by_words(int argc, char *argv[]) {
  if (argc < 4) {
    fprintf(stdout, "Usage: %s log search <word1> <word2> ... <wordN>\n",
            argv[0]);
    return;
  }

  // Calculate the number of words provided for search
  int num_words = argc - 3;

  // Open message.txt for reading
  FILE *message_file = fopen(".sit/Messages.txt", "r");
  if (message_file == NULL) {
    fprintf(stderr, "Error opening Messages.txt file.\n");
    exit(EXIT_FAILURE);
  }

  char line[1024];
  int total_lines = 0;
  int word_found = 0; // Flag to check if the search word is found

  // Count the total number of lines in message.txt
  while (fgets(line, sizeof(line), message_file) != NULL) {
    total_lines++;
  }
  // printf("%d", total_lines);
  //  Rewind to the beginning of the file
  rewind(message_file);

  // Iterate through each word in the search
  for (int word_index = 3; word_index < argc; word_index++) {
    rewind(message_file);
    const char *search_word = argv[word_index];
    word_found = 0; // Reset the flag for each search word

    // Iterate through each line in message.txt
    for (int line_number = 0; line_number < total_lines; line_number++) {
      if (fgets(line, sizeof(line), message_file) != NULL) {
        // Check if the line contains the search word
        if (strstr(line, search_word) != NULL) {
          // Calculate commit number based on total_lines and line_number
          int commit_number = total_lines - line_number;
          // printf("%d", commit_number);
          //  Search for the commit in each branch
          DIR *branches_dir = opendir(".sit/branches");
          if (branches_dir == NULL) {
            fprintf(stderr, "Error opening branches directory.\n");
            exit(EXIT_FAILURE);
          }

          struct dirent *branch_entry;

          // Iterate through each branch
          while ((branch_entry = readdir(branches_dir)) != NULL) {
            if (branch_entry->d_type == DT_DIR) {
              const char *branch_name = branch_entry->d_name;

              if (strcmp(branch_name, ".") != 0 &&
                  strcmp(branch_name, "..") != 0) {
                // Construct the path to the commit folder in the branch
                char commit_folder_path[MAX_FILENAME_LENGTH];
                snprintf(commit_folder_path, sizeof(commit_folder_path),
                         ".sit/branches/%s/%d", branch_name, commit_number);

                // Check if the commit folder exists
                if (access(commit_folder_path, F_OK) == 0) {
                  // Open and print info.txt from the commit folder
                  char info_file_path[MAX_FILENAME_LENGTH];
                  snprintf(info_file_path, sizeof(info_file_path),
                           "%s/info.txt", commit_folder_path);
                  FILE *info_file = fopen(info_file_path, "r");

                  if (info_file == NULL) {
                    fprintf(
                        stderr,
                        "Error opening info.txt file in commit folder %s.\n",
                        commit_folder_path);
                    exit(EXIT_FAILURE);
                  }

                  // Read and print the content of info.txt
                  char buffer[1024];
                  while (fgets(buffer, sizeof(buffer), info_file) != NULL) {
                    printf("%s", buffer);
                  }

                  fclose(info_file);
                  word_found = 1; // Set the flag indicating the word is found
                  break; // Break after finding the commit in the current branch
                }
              }
            }
          }

          closedir(branches_dir);

          // Break the loop for other lines if the word is found
          if (word_found) {
            break;
          }
        }
      }
    }

    // Print an error message if the word is not found
    if (!word_found) {
      fprintf(stderr, "Error: Word '%s' not found in any commit message.\n",
              search_word);
      break; // Exit the loop if any word is not found
    }
  }

  fclose(message_file);
}
void list_branches() {
  // Open the branches directory
  DIR *branches_dir = opendir(".sit/branches");
  if (branches_dir == NULL) {
    fprintf(stderr, "Error opening branches directory.\n");
    return;
  }

  struct dirent *branch_entry;

  // Print the names of all folders in the branches directory
  while ((branch_entry = readdir(branches_dir)) != NULL) {
    if (branch_entry->d_type == DT_DIR) {
      const char *branch_name = branch_entry->d_name;

      if (strcmp(branch_name, ".") != 0 && strcmp(branch_name, "..") != 0) {
        printf("%s\n", branch_name);
      }
    }
  }

  closedir(branches_dir);
}
int run_branch(int argc, char *argv[]) {
  if (argc == 2) {
    // If no branch name provided, list existing branches
    list_branches();
  } else if (argc == 3) {
    // If branch name is provided, create a new branch
    const char *branch_name = argv[2];

    // Check if the .sit folder exists
    char sit_folder_path[MAX_FILENAME_LENGTH];
    snprintf(sit_folder_path, sizeof(sit_folder_path), ".sit");
    if (access(sit_folder_path, F_OK) == -1) {
      fprintf(stderr, "Error: .sit not found. Did you run 'sit init'?\n");
      return 1;
    }

    // Check if the branch folder already exists
    char branch_folder_path[MAX_FILENAME_LENGTH];
    snprintf(branch_folder_path, sizeof(branch_folder_path), ".sit/branches/%s",
             branch_name);
    if (access(branch_folder_path, F_OK) == 0) {
      fprintf(stderr, "Error: Branch '%s' already exists.\n", branch_name);
      return 1;
    }

    // Create the new branch folder
    if (mkdir(branch_folder_path, 0755) != 0) {
      fprintf(stderr, "Error creating branch folder.\n");
      return 1;
    }

    printf("Branch '%s' created successfully!\n", branch_name);
    return 0;
  } else {
    fprintf(stdout, "Usage: %s branch [<branch-name>]\n", argv[0]);
    return 1;
  }
}

void cleanup_fop_folder() {
  DIR *fop_dir = opendir("/Users/nazaninqaffari/desktop/fop");
  if (fop_dir == NULL) {
    fprintf(stderr, "Error opening fop directory.\n");
    exit(EXIT_FAILURE);
  }
  struct dirent *entry;
  char file_path[MAX_FILENAME_LENGTH];

  while ((entry = readdir(fop_dir)) != NULL) {
    const char *file_name = entry->d_name;
    if (strcmp(file_name, ".") != 0 && strcmp(file_name, "..") != 0 &&
        strcmp(file_name, ".sit") != 0 && strcmp(file_name, "sit.c") != 0 &&
        strcmp(file_name, "sit") != 0) {

      snprintf(file_path, sizeof(file_path),
               "/Users/nazaninqaffari/desktop/fop/%s", file_name);
      remove(file_path);
    }
  }
  closedir(fop_dir);
}
void copy_file_unix(const char *source, const char *destination) {
  FILE *source_file = fopen(source, "rb");
  FILE *destination_file = fopen(destination, "wb");

  if (source_file == NULL || destination_file == NULL) {
    fprintf(stderr, "Error opening source or destination file.\n");
    exit(EXIT_FAILURE);
  }

  char buffer[1024];
  size_t bytesRead;

  while ((bytesRead = fread(buffer, 1, sizeof(buffer), source_file)) > 0) {
    fwrite(buffer, 1, bytesRead, destination_file);
  }

  fclose(source_file);
  fclose(destination_file);
}

void copy_files(const char *source_folder, const char *destination_folder) {
  DIR *source_dir = opendir(source_folder);
  if (source_dir == NULL) {
    fprintf(stderr, "Error opening source directory.\n");
    exit(EXIT_FAILURE);
  }
  DIR *destination_dir = opendir(destination_folder);
  if (destination_dir == NULL) {
    fprintf(stderr, "Error opening destination directory.\n");
    closedir(source_dir);
    exit(EXIT_FAILURE);
  }
  struct dirent *entry;
  char source_file_path[MAX_FILENAME_LENGTH];
  char destination_file_path[MAX_FILENAME_LENGTH];

  while ((entry = readdir(source_dir)) != NULL) {
    const char *file_name = entry->d_name;

    if (strcmp(file_name, ".") != 0 && strcmp(file_name, "..") != 0 &&
        strcmp(file_name, "stagedOrNot") != 0 &&
        strcmp(file_name, "info.txt") != 0 &&
        strcmp(file_name, "resetfiles.txt") != 0) {

      snprintf(source_file_path, sizeof(source_file_path), "%s/%s",
               source_folder, file_name);
      snprintf(destination_file_path, sizeof(destination_file_path), "%s/%s",
               destination_folder, file_name);
      copy_file_unix(source_file_path, destination_file_path);
    }
  }

  closedir(source_dir);
  closedir(destination_dir);
}
int isNumber(const char *str) {
  while (*str) {
    if (!isdigit(*str)) {
      return 0; // Not a number
    }
    str++; // Move to the next character
  }
  return 1; // All characters are digits, so it's a number
}
// Function to compare the contents of two files
int compare_two_files(const char *file1_path, const char *file2_path) {
  FILE *file1 = fopen(file1_path, "r");
  FILE *file2 = fopen(file2_path, "r");

  if (file1 == NULL || file2 == NULL) {
    fprintf(stderr, "Error opening files for comparison.\n");
    return 0; // Comparison failed
  }

  char line1[1024], line2[1024];

  while (fgets(line1, sizeof(line1), file1) != NULL) {
    if (fgets(line2, sizeof(line2), file2) == NULL ||
        strcmp(line1, line2) != 0) {
      fclose(file1);
      fclose(file2);
      return 0; // Lines are not the same
    }
  }

  // Check if file2 has additional lines
  if (fgets(line2, sizeof(line2), file2) != NULL) {
    fclose(file1);
    fclose(file2);
    return 0; // Lines are not the same
  }

  fclose(file1);
  fclose(file2);

  return 1; // Files are the same
}
int compare_files(const char *dir1, const char *dir2) {
  DIR *dir1_ptr = opendir(dir1);
  DIR *dir2_ptr = opendir(dir2);

  if (dir1_ptr == NULL || dir2_ptr == NULL) {
    fprintf(stderr, "Error opening directories for comparison.\n");
    return 0; // Comparison failed
  }

  struct dirent *entry1, *entry2;

  while ((entry1 = readdir(dir1_ptr)) != NULL) {
    if (entry1->d_type == DT_REG && strcmp(entry1->d_name, "sit.c") != 0 &&
        strcmp(entry1->d_name, ".sit") != 0 &&
        strcmp(entry1->d_name, "sit") != 0 &&
        strcmp(entry1->d_name, "stagedOrNot") != 0 &&
        strcmp(entry1->d_name, "resetfiles.txt") != 0 &&
        strcmp(entry1->d_name, "info.txt") != 0 &&
        strcmp(entry1->d_name, ".") != 0 && strcmp(entry1->d_name, "..") != 0 &&
        strcmp(entry1->d_name, ".DS_Store") != 0) { // Regular file
      char file1_path[MAX_FILENAME_LENGTH];
      snprintf(file1_path, sizeof(file1_path), "%s/%s", dir1, entry1->d_name);
      char file2_path[MAX_FILENAME_LENGTH];
      snprintf(file2_path, sizeof(file2_path), "%s/%s", dir2, entry1->d_name);

      if (!compare_two_files(file1_path, file2_path)) {
        closedir(dir1_ptr);
        closedir(dir2_ptr);
        return 0; // Files are not the same
      }
    }
  }

  closedir(dir1_ptr);
  closedir(dir2_ptr);

  return 1; // Files are the same
}

int count_files(const char *dir_path) {
  int count = 0;
  DIR *dir = opendir(dir_path);
  if (dir != NULL) {
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
      if (entry->d_type == DT_REG && strcmp(entry->d_name, "sit.c") != 0 &&
          strcmp(entry->d_name, ".sit") != 0 &&
          strcmp(entry->d_name, "sit") != 0 &&
          strcmp(entry->d_name, "stagedOrNot") != 0 &&
          strcmp(entry->d_name, "resetfiles.txt") != 0 &&
          strcmp(entry->d_name, "info.txt") != 0 &&
          strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0 &&
          strcmp(entry->d_name, ".DS_Store") != 0) { // Regular file
        // printf("%s" ,entry->d_name );
        count++;
      }
    }
    closedir(dir);
  }
  // printf("%d", count);
  return count;
}

int run_checkout(const char *target) {
  // Check if .sit folder exists
  char sit_folder_path[MAX_FILENAME_LENGTH];
  snprintf(sit_folder_path, sizeof(sit_folder_path), ".sit");
  if (access(sit_folder_path, F_OK) == -1) {
    fprintf(stderr, "Error: .sit not found. Did you run 'sit init'?\n");
    return 1;
  }
  // Read the last commit number from commit_number.txt
  int last_commit_number = read_latest_commit();
  last_commit_number--;
  int commit_number = read_commit_number();
  commit_number--;
  // Check each branch for a commit folder with the specified number
  DIR *branches_dir = opendir(".sit/branches");
  if (branches_dir == NULL) {
    fprintf(stderr, "Error opening branches directory.\n");
    return 1;
  }
  struct dirent *branch_entry;
  int found_commit = 0;

  while ((branch_entry = readdir(branches_dir)) != NULL) {
    if (branch_entry->d_type == DT_DIR) {
      const char *branch_name = branch_entry->d_name;

      if (strcmp(branch_name, ".") != 0 && strcmp(branch_name, "..") != 0) {
        char commit_folder_path[MAX_FILENAME_LENGTH];
        if (strcmp(target, "HEAD") == 0) {
          snprintf(commit_folder_path, sizeof(commit_folder_path),
                   ".sit/branches/%s/%d", branch_name, last_commit_number);
          char target_buffer[MAX_FILENAME_LENGTH];
          snprintf(target_buffer, sizeof(target_buffer), "%d", commit_number);
          strcpy(target, target_buffer);
        } else {
          snprintf(commit_folder_path, sizeof(commit_folder_path),
                   ".sit/branches/%s/%d", branch_name, last_commit_number);
        }
        // Check if the commit folder exists
        if (access(commit_folder_path, F_OK) == 0) {
          // Verify the number of files in the commit folder and fop folder
          // (Minus 2 for excluding . and ..)
          int num_files_commit = count_files(commit_folder_path);
          int num_files_fop = count_files("/Users/nazaninqaffari/desktop/fop");

          if (num_files_commit == num_files_fop) {
            // Compare each file in the commit folder with the corresponding
            // file in fop
            if (compare_files(commit_folder_path,
                              "/Users/nazaninqaffari/desktop/fop")) {
              found_commit = 1;
              break; // Break after finding the correct commit
            } else {
              fprintf(stderr, "Error: Files in commit folder and fop folder "
                              "are not the same.please commit first. \n");
              return 1;
            }
          } else {
            fprintf(stderr, "Error: Number of files in commit folder and fop "
                            "folder are different.please commit first . \n");
            return 1;
          }
        }
      }
    }
  }

  closedir(branches_dir);

  if (found_commit) {
    cleanup_fop_folder();
    if (isNumber(target)) {
      DIR *branches_dir1 = opendir(".sit/branches");
      struct dirent *branch_entry1;
      while ((branch_entry1 = readdir(branches_dir1)) != NULL) {
        if (branch_entry1->d_type == DT_DIR) {
          const char *branch_name1 = branch_entry1->d_name;
          if (strcmp(branch_name1, ".") != 0 &&
              strcmp(branch_name1, "..") != 0) {
            char commit_folder_path1[MAX_FILENAME_LENGTH];
            snprintf(commit_folder_path1, sizeof(commit_folder_path1),
                     ".sit/branches/%s/%s", branch_name1, target);

            // Check if the commit folder exists
            if (access(commit_folder_path1, F_OK) == 0) {
              copy_files(commit_folder_path1,
                         "/Users/nazaninqaffari/desktop/fop");
              printf("Checkout successful!\n");
              update_latest_commit(atoi(target) + 1);
              if (check)
                write_commit_number(commit_number + 2);
              return 0;
            }
            // Copy all files from the commit folder to fop (excluding
            // stageornot.txt and info.txt)
          }
        }
      }
    } else {
      char max[1000];
      char maxxx[1000];
      DIR *branches_dir2 = opendir(".sit/branches");
      struct dirent *branch_entry2;
      while ((branch_entry2 = readdir(branches_dir2)) != NULL) {
        if (strcmp(branch_entry2->d_name, target) == 0) {
          char branch_path[MAX_FILENAME_LENGTH];
          snprintf(branch_path, sizeof(branch_path), ".sit/branches/%s",
                   branch_entry2->d_name);
          DIR *branches_dir3 = opendir(branch_path);
          struct dirent *entry3;

          while ((entry3 = readdir(branches_dir3)) != NULL) {
            strcpy(max, entry3->d_name);
            if (atoi(max) > atoi(maxxx)) {
              strcpy(maxxx, max);
            }
          }
          char thisfolder[150];
          snprintf(thisfolder, sizeof(thisfolder), "%s/%s", branch_path, max);
          copy_files(thisfolder, "/Users/nazaninqaffari/desktop/fop");
          printf("Checkout successful!\n");
          update_latest_commit(atoi(target) + 1);
          if (check)
            write_commit_number(commit_number + 2);
        }
      }
    }
  } else {
    fprintf(stderr, "Error: No valid commit found for checkout.\n");
    return 1;
  }
}
// Function to print the status of files in fop compared to the commit folder
void print_status(const char *commit_folder, const char *fop_folder) {
  DIR *fop_dir = opendir(fop_folder);
  if (fop_dir == NULL) {
    fprintf(stderr, "Error opening FOP directory.\n");
    exit(EXIT_FAILURE);
  }

  struct dirent *entry;
  char file_path[MAX_FILENAME_LENGTH];

  while ((entry = readdir(fop_dir)) != NULL) {
    const char *file_name = entry->d_name;
    if (strcmp(file_name, ".") != 0 && strcmp(file_name, "..") != 0 &&
        strcmp(file_name, ".DS_Store") != 0 && strcmp(file_name, ".sit") != 0 &&
        strcmp(file_name, "sit") != 0 && strcmp(file_name, "sit.c") != 0) {
      snprintf(file_path, sizeof(file_path), "%s/%s", fop_folder, file_name);

      // Check if the file exists in the commit folder
      char commit_file_path[MAX_FILENAME_LENGTH];
      snprintf(commit_file_path, sizeof(commit_file_path), "%s/%s",
               commit_folder, file_name);

      // Check if the file exists in the add folder
      char add_file_path[MAX_FILENAME_LENGTH];
      snprintf(add_file_path, sizeof(add_file_path), "%s/%s", ".sit/add",
               file_name);

      // Compare file contents
      int exists_in_commit_folder = (access(commit_file_path, F_OK) == 0);
      int exists_in_add_folder = (access(add_file_path, F_OK) == 0);

      if (exists_in_add_folder) {
        if (exists_in_commit_folder) {
          int compare_files = compare_two_files(file_path, commit_file_path);
          if (compare_files) {
            printf("%s is : + NM\n", file_name);
          } else {
            printf("%s is : + M\n", file_name);
          }
        } else {
          printf("%s is : +A\n", file_name);
        }
      } else {
        if (exists_in_commit_folder) {
          int compare_files = compare_two_files(file_path, commit_file_path);
          if (compare_files) {
            printf("%s is : - NM\n", file_name);
          } else {
            printf("%s is : - M\n", file_name);
          }
        } else {
          printf("%s is : -A\n", file_name);
        }
      }
    }
  }
  // Check if there are files in commit folder that are not in FOP
  DIR *commit_dir = opendir(commit_folder);
  if (commit_dir == NULL) {
    fprintf(stderr, "Error opening commit directory.\n");
    exit(EXIT_FAILURE);
  }

  while ((entry = readdir(commit_dir)) != NULL) {
    const char *file_name = entry->d_name;
    if (strcmp(file_name, ".") != 0 && strcmp(file_name, "..") != 0 &&
        strcmp(file_name, ".DS_Store") != 0 &&
        strcmp(file_name, "stagedOrNot") != 0 &&
        strcmp(file_name, "info.txt") != 0 &&
        strcmp(file_name, "resetfiles.txt") != 0) {
      char commit_file_path[MAX_FILENAME_LENGTH];
      snprintf(commit_file_path, sizeof(commit_file_path), "%s/%s",
               commit_folder, file_name);
      char fop_folder_file[MAX_FILENAME_LENGTH];
      snprintf(fop_folder_file, sizeof(fop_folder_file), "%s/%s",
               "/Users/nazaninqaffari/desktop/fop", file_name);
      char fop_add_file[MAX_FILENAME_LENGTH];
      snprintf(fop_add_file, sizeof(fop_add_file), "%s/%s", ".sit/add",
               file_name);
      // Check if the file in commit folder is not in FOP
      if (access(fop_folder_file, F_OK) == -1) {
        if (access(fop_add_file, F_OK) == -1) {
          printf("%s is : -D\n", file_name);
        } else {
          printf("%s is : +D\n", file_name);
        } // File is in commit folder but not in FOP or add folder
      }
    }
  }

  closedir(fop_dir);
  closedir(commit_dir);
}

int run_status() {
  // Read the last commit number from commit_number.txt
  int last_commit_number = read_commit_number();
  last_commit_number--;

  // Check each branch for a commit folder with the specified number
  DIR *branches_dir = opendir(".sit/branches");
  if (branches_dir == NULL) {
    fprintf(stderr, "Error opening branches directory.\n");
    return 1;
  }

  struct dirent *branch_entry;
  int found_commit = 0;

  while ((branch_entry = readdir(branches_dir)) != NULL) {
    if (branch_entry->d_type == DT_DIR) {
      const char *branch_name = branch_entry->d_name;

      if (strcmp(branch_name, ".") != 0 && strcmp(branch_name, "..") != 0) {

        // Read the commit number from commit_number.txt in each branch
        char commit_number_path[MAX_FILENAME_LENGTH];
        snprintf(commit_number_path, sizeof(commit_number_path),
                 ".sit/branches/%s/%d", branch_name, last_commit_number);

        // Check if the commit folder exists
        if (access(commit_number_path, F_OK) == 0) {
          found_commit = 1;
          print_status(commit_number_path, "/Users/nazaninqaffari/desktop/fop");
          break; // Break after finding the correct commit
        }
      }
    }
  }

  closedir(branches_dir);

  if (!found_commit) {
    fprintf(stderr, "Error: No valid commit found for status.\n");
    return 1;
  }

  return 0;
}
int commit_folder_exists(const char *commit_id) {
  DIR *dir;
  struct dirent *entry;
  char branches_path[MAX_FILENAME_LENGTH];
  snprintf(branches_path, sizeof(branches_path), ".sit/branches");

  if ((dir = opendir(branches_path)) != NULL) {
    while ((entry = readdir(dir)) != NULL) {
      if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 &&
          strcmp(entry->d_name, "..") != 0) {
        char commit_folder_path[MAX_FILENAME_LENGTH];
        snprintf(commit_folder_path, sizeof(commit_folder_path), "%s/%s",
                 branches_path, entry->d_name);

        if (access(commit_folder_path, F_OK) != -1) {
          char commit_id_path[MAX_FILENAME_LENGTH];
          snprintf(commit_id_path, sizeof(commit_id_path), "%s/%s",
                   commit_folder_path, commit_id);
          if (access(commit_id_path, F_OK) != -1) {
            closedir(dir);
            return 1; // Commit folder found in this branch
          }
        }
      }
    }
    closedir(dir);
  }
  return 0; // Commit folder not found in any branch
}

int run_revert(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "Usage: %s revert [-m \"message\"] <commit-id>\n", argv[0]);
    return 1;
  }
  if (argc < 3) {
    fprintf(stderr, "Usage: %s revert [-m \"message\"] [-n] [<commit-id>]\n",
            argv[0]);
    return 1;
  }

  const char *commit_id = "";
  const char *message = ""; // Default message if not provided

  int use_default_message = 1; // Assume using default message
  int only_checkout = 0; // Flag to indicate whether only checkout is needed

  // Check for optional flags
  for (int i = 2; i < argc; i++) {
    if (strcmp(argv[i], "-m") == 0) {
      message = argv[i + 1];
      use_default_message = 0;
      i++;
    } else if (strcmp(argv[i], "-n") == 0) {
      commit_id = argv[i + 1];
      only_checkout = 1;
      break;
    } else {
      commit_id = argv[i];
    }
  }

  // Check if the .sit folder exists
  char sit_folder_path[MAX_FILENAME_LENGTH];
  snprintf(sit_folder_path, sizeof(sit_folder_path), ".sit");
  if (access(sit_folder_path, F_OK) == -1) {
    fprintf(stderr, "Error: .sit not found. Did you run 'sit init'?\n");
    return 1;
  }
  if (only_checkout) {
    if (commit_id == NULL) {
      // Read the commit number from commit_number.txt
      int last_commit_number = read_commit_number();
      last_commit_number--;

      // Generate commit id from the commit number
      char commit_id_from_txt[100];
      snprintf(commit_id_from_txt, sizeof(commit_id_from_txt), "%d",
               last_commit_number);

      // Run checkout with the generated commit id
      return run_checkout(commit_id_from_txt);
    } else {
      // Run checkout with the provided commit id
      return run_checkout(commit_id);
    }
  }
  // Check if the specified commit folder exists in any branch
  DIR *dir = opendir(".sit/branches");
  struct dirent *entry;
  char branches_path[MAX_FILENAME_LENGTH];
  snprintf(branches_path, sizeof(branches_path), ".sit/branches");
  char commit_id_path[MAX_FILENAME_LENGTH];
  char commit_new_path[MAX_FILENAME_LENGTH];
  char branchname[100];
  if ((dir = opendir(branches_path)) != NULL) {
    while ((entry = readdir(dir)) != NULL) {
      if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 &&
          strcmp(entry->d_name, "..") != 0) {
        strcpy(branchname, entry->d_name);
        char commit_folder_path[MAX_FILENAME_LENGTH];
        snprintf(commit_folder_path, sizeof(commit_folder_path), "%s/%s",
                 branches_path, entry->d_name);

        if (access(commit_folder_path, F_OK) != -1) {
          snprintf(commit_id_path, sizeof(commit_id_path), "%s/%s",
                   commit_folder_path, commit_id);
          snprintf(commit_new_path, sizeof(commit_new_path), "%s",
                   commit_folder_path);
          if (access(commit_id_path, F_OK) != -1) {
            closedir(dir);

            // Continue with creating a new commit folder for the revert
            // Get the commit number for the new commit
            int new_commit_number = read_commit_number();
            // write_commit_number(new_commit_number + 1);
            //  Create the new commit folder
            char new_commit_number_str[1000];
            snprintf(new_commit_number_str, sizeof(new_commit_number_str),
                     "%s/%d", commit_new_path, new_commit_number);
            if (mkdir(new_commit_number_str, 0755) != 0) {
              fprintf(stderr, "Error creating new commit folder.\n");
              return 1;
            }

            // Copy files from the specified commit folder to the new commit
            // folder
            copy_files(commit_id_path, new_commit_number_str);

            // Update the info.txt file in the new commit folder
            if (use_default_message) {
              // If using default message, extract the message from the commit
              // folder
              char info_path[MAX_FILENAME_LENGTH];
              snprintf(info_path, sizeof(info_path), "%s/info.txt",
                       commit_id_path);
              FILE *info_file = fopen(info_path, "r");
              if (info_file != NULL) {
                char line[MAX_FILENAME_LENGTH];
                while (fgets(line, sizeof(line), info_file) != NULL) {
                  if (strstr(line, "Commit Message:") != NULL) {
                    // Found the line with the commit message
                    const char *message_prefix = "Commit Message: ";
                    message = line + strlen(message_prefix);
                    break;
                  }
                }
                fclose(info_file);
              }
            }

            update_file_info(new_commit_number_str, new_commit_number,
                             count_files(new_commit_number_str), branchname,
                             "Nazi", message);

            // Update commit number and latest commit number

            printf("Revert was successful!\n");
            check = 1;
            // Now perform the checkout
            return run_checkout(commit_id);
          }
        }
      }
    }
  } else {
    fprintf(stderr, "Error: Commit folder '%s' not found.\n", commit_id);
    return 1;
  }
  fprintf(stderr, "Error: Commit folder '%s' not found.\n", commit_id);
  return 1;
}
int run_revert2(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "Usage: %s revert [-m \"message\"] <HEAD-X>\n", argv[0]);
    return 1;
  }

  const char *commit_id = "";
  const char *message = ""; // Default message if not provided

  int use_default_message = 1; // Assume using default message

  // Check for optional message flag
  for (int i = 2; i < argc; i++) {
    if (strcmp(argv[i], "-m") == 0) {
      message = argv[i + 1];
      use_default_message = 0;
      i++;
    } else {
      commit_id = argv[i];
    }
  }
  if (strstr(commit_id, "HEAD") != NULL) {
    // Read the commit number from commit_number.txt
    int last_commit_number = read_commit_number();
    last_commit_number--;
    int N;
    // Extract the number N from HEAD-(N)
    if (use_default_message == 0) {
      N = atoi(strstr(argv[4], "HEAD-") + strlen("HEAD-"));
    } else {
      N = atoi(strstr(argv[2], "HEAD-") + strlen("HEAD-"));
    }
    // Calculate the target commit number
    int target_commit_number = last_commit_number - N;

    // Create a buffer to hold the string representation of target_commit_number
    char target_commit_str[100];
    snprintf(target_commit_str, sizeof(target_commit_str), "%d",
             target_commit_number);

    // Set commit_id using the buffer
    commit_id = strdup(target_commit_str);
  }
  // Check if the .sit folder exists
  char sit_folder_path[MAX_FILENAME_LENGTH];
  snprintf(sit_folder_path, sizeof(sit_folder_path), ".sit");
  if (access(sit_folder_path, F_OK) == -1) {
    fprintf(stderr, "Error: .sit not found. Did you run 'sit init'?\n");
    return 1;
  }

  // Check if the specified commit folder exists in any branch
  DIR *dir = opendir(".sit/branches");
  struct dirent *entry;
  char branches_path[MAX_FILENAME_LENGTH];
  snprintf(branches_path, sizeof(branches_path), ".sit/branches");
  char commit_id_path[MAX_FILENAME_LENGTH];
  char commit_new_path[MAX_FILENAME_LENGTH];
  char branchname[100];
  if ((dir = opendir(branches_path)) != NULL) {
    while ((entry = readdir(dir)) != NULL) {
      if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 &&
          strcmp(entry->d_name, "..") != 0) {
        strcpy(branchname, entry->d_name);
        char commit_folder_path[MAX_FILENAME_LENGTH];
        snprintf(commit_folder_path, sizeof(commit_folder_path), "%s/%s",
                 branches_path, entry->d_name);

        if (access(commit_folder_path, F_OK) != -1) {
          snprintf(commit_id_path, sizeof(commit_id_path), "%s/%s",
                   commit_folder_path, commit_id);
          snprintf(commit_new_path, sizeof(commit_new_path), "%s",
                   commit_folder_path);
          if (access(commit_id_path, F_OK) != -1) {
            closedir(dir);

            // Continue with creating a new commit folder for the revert
            // Get the commit number for the new commit
            int new_commit_number = read_commit_number();
            // write_commit_number(new_commit_number + 1);
            //  Create the new commit folder
            char new_commit_number_str[1000];
            snprintf(new_commit_number_str, sizeof(new_commit_number_str),
                     "%s/%d", commit_new_path, new_commit_number);
            if (mkdir(new_commit_number_str, 0755) != 0) {
              fprintf(stderr, "Error creating new commit folder.\n");
              return 1;
            }

            // Copy files from the specified commit folder to the new commit
            // folder
            copy_files(commit_id_path, new_commit_number_str);

            // Update the info.txt file in the new commit folder
            if (use_default_message) {
              // If using default message, extract the message from the commit
              // folder
              char info_path[MAX_FILENAME_LENGTH];
              snprintf(info_path, sizeof(info_path), "%s/info.txt",
                       commit_id_path);
              FILE *info_file = fopen(info_path, "r");
              if (info_file != NULL) {
                char line[MAX_FILENAME_LENGTH];
                while (fgets(line, sizeof(line), info_file) != NULL) {
                  if (strstr(line, "Commit Message:") != NULL) {
                    // Found the line with the commit message
                    const char *message_prefix = "Commit Message: ";
                    message = line + strlen(message_prefix);
                    break;
                  }
                }
                fclose(info_file);
              }
            }

            update_file_info(new_commit_number_str, new_commit_number,
                             count_files(new_commit_number_str), branchname,
                             "Nazi", message);

            // Update commit number and latest commit number

            printf("Revert was successful!\n");
            check = 1;
            // Now perform the checkout
            return run_checkout(commit_id);
          }
        }
      }
    }
  } else {
    fprintf(stderr, "Error: Commit folder '%s' not found.\n", commit_id);
    return 1;
  }
  fprintf(stderr, "Error: Commit folder '%s' not found.\n", commit_id);
  return 1;
}
// Function to get the author from config file
void get_author(char *author) {
  char config_path[MAX_FILENAME_LENGTH];
  snprintf(config_path, sizeof(config_path), ".sit/config/%sconfig.txt",
           (access(".sit/config/localconfig.txt", F_OK) == 0) ? "local"
                                                              : "global");

  FILE *config_file = fopen(config_path, "r");
  if (config_file == NULL) {
    fprintf(stderr, "Error opening config file.\n");
    exit(EXIT_FAILURE);
  }

  // Read the first two lines from the config file
  if (fgets(author, MAX_FILENAME_LENGTH, config_file) == NULL) {
    fprintf(stderr, "Error reading author name from config file.\n");
    fclose(config_file);
    exit(EXIT_FAILURE);
  }

  // Remove the newline character at the end, if present
  size_t len = strlen(author);
  if (len > 0 && author[len - 1] == '\n') {
    author[len - 1] = '\0';
  }

  // Read the second line (email)
  char email[MAX_FILENAME_LENGTH];
  if (fgets(email, MAX_FILENAME_LENGTH, config_file) == NULL) {
    fprintf(stderr, "Error reading author email from config file.\n");
    fclose(config_file);
    exit(EXIT_FAILURE);
  }

  // Remove the newline character at the end, if present
  len = strlen(email);
  if (len > 0 && email[len - 1] == '\n') {
    email[len - 1] = '\0';
  }

  // Concatenate name and email into the author string
  snprintf(author, MAX_FILENAME_LENGTH, "%s <%s>", author, email);
  // Close the config file
  fclose(config_file);
}

void get_current_timestamp(char *timestamp) {
  time_t rawtime;
  struct tm *timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  strftime(timestamp, MAX_FILENAME_LENGTH, "%a %b %d %H:%M:%S %Y", timeinfo);
}
int run_tag(int argc, char *argv[]) {
  if (argc < 4 || argc > 9) {
    fprintf(
        stderr,
        "Usage: %s tag -a <tag-name> [-m <message>] [-c <commit-id>] [-f]\n",
        argv[0]);
    return 1;
  }

  char *tag_name = NULL;
  char *message = NULL;
  char *commit_id = NULL;
  int force_flag = 0;

  // Parse command-line arguments
  for (int i = 2; i < argc; i++) {
    if (strcmp(argv[i], "-a") == 0) {
      tag_name = argv[i + 1];
      i++;
    } else if (strcmp(argv[i], "-m") == 0) {
      message = argv[i + 1];
      i++;
    } else if (strcmp(argv[i], "-c") == 0) {
      commit_id = argv[i + 1];
      i++;
    } else if (strcmp(argv[i], "-f") == 0) {
      force_flag = 1;
    }
  }

  if (tag_name == NULL) {
    fprintf(stderr, "Error: Tag name is required.\n");
    return 1;
  }

  // Check if .sit/tags folder exists, create if not
  char tags_folder_path[MAX_FILENAME_LENGTH];
  snprintf(tags_folder_path, sizeof(tags_folder_path), ".sit/tags");
  if (access(tags_folder_path, F_OK) == -1) {
    if (mkdir(tags_folder_path, 0755) != 0) {
      fprintf(stderr, "Error creating tags folder.\n");
      return 1;
    }
  }

  // Determine the commit ID
  if (commit_id == NULL) {
    // Read the commit number from commit_number.txt
    int last_commit_number = read_commit_number();
    last_commit_number--;

    // Generate commit id from the commit number
    char commit_id_buffer[100];
    snprintf(commit_id_buffer, sizeof(commit_id_buffer), "%d",
             last_commit_number);
    commit_id = strdup(commit_id_buffer);
  }

  // Create tag information
  char tag_info[MAX_FILENAME_LENGTH];
  snprintf(tag_info, sizeof(tag_info), "%s\ncommit %s\n", tag_name, commit_id);

  // Add author information
  char author_info[MAX_FILENAME_LENGTH];
  get_author(author_info);
  strcat(tag_info, "author: ");
  strcat(tag_info, author_info);
  strcat(tag_info, "\n");
  // Add timestamp
  char timestamp_info[MAX_FILENAME_LENGTH];
  get_current_timestamp(timestamp_info);
  strcat(tag_info, "Date: ");
  strcat(tag_info, timestamp_info);
  strcat(tag_info, "\n");

  // Add message if provided
  if (message != NULL) {
    strcat(tag_info, "Message: ");
    strcat(tag_info, message);
    strcat(tag_info, "\n");
  }

  // Create tag file
  char tag_file_path[MAX_FILENAME_LENGTH];
  snprintf(tag_file_path, sizeof(tag_file_path), "%s/%s.txt", tags_folder_path,
           tag_name);

  // Check if tag file already exists (and force flag not set)
  if (access(tag_file_path, F_OK) == 0 && !force_flag) {
    fprintf(stderr,
            "Error: Tag '%s' already exists. Use -f to force overwrite.\n",
            tag_name);
    return 1;
  }

  FILE *tag_file = fopen(tag_file_path, "w");
  if (tag_file == NULL) {
    fprintf(stderr, "Error creating tag file.\n");
    return 1;
  }

  // Write tag information to the file
  fprintf(tag_file, "%s", tag_info);

  // Close the file
  fclose(tag_file);
  printf("Tag '%s' created successfully.\n", tag_name);
  return 0;
}
int compare_strings(const void *a, const void *b) {
  return strcmp(*(const char **)a, *(const char **)b);
}
void list_tags() {
  char tags_folder_path[MAX_FILENAME_LENGTH];
  snprintf(tags_folder_path, sizeof(tags_folder_path), ".sit/tags");
  // Check if tags folder exists
  if (access(tags_folder_path, F_OK) == -1) {
    fprintf(stderr, "No tags found.\n");
    return;
  }

  // Open the tags folder
  DIR *dir = opendir(tags_folder_path);
  if (dir == NULL) {
    fprintf(stderr, "Error opening tags folder.\n");
    return;
  }

  // Read tags from the directory and store in an array
  struct dirent *entry;
  char tag_file_path[MAX_FILENAME_LENGTH];
  char *tag_names[100]; // Assuming there are at most 100 tags
  int num_tags = 0;

  while ((entry = readdir(dir)) != NULL) {
    // Skip entries starting with dot (.) and subdirectories
    if (entry->d_name[0] == '.') {
      continue;
    }
    // Store tag names
    tag_names[num_tags] = strdup(entry->d_name);
    num_tags++;
  }

  // Sort tag names alphabetically
  qsort(tag_names, num_tags, sizeof(char *), compare_strings);

  // Display tag information
  for (int i = 0; i < num_tags; i++) {
    snprintf(tag_file_path, sizeof(tag_file_path), "%s/%s", tags_folder_path,
             tag_names[i]);

    FILE *tag_file = fopen(tag_file_path, "r");
    if (tag_file != NULL) {
      char tag_info[MAX_FILENAME_LENGTH];
      while (fgets(tag_info, sizeof(tag_info), tag_file) != NULL) {
        printf("%s", tag_info); // Display tag information
      }
      fclose(tag_file);
    }
    free(tag_names[i]); // Free allocated memory for tag names
  }
  // Close the directory
  closedir(dir);
}
void view_tag_info(const char *tag_name) {
  char tags_folder_path[MAX_FILENAME_LENGTH];
  snprintf(tags_folder_path, sizeof(tags_folder_path), ".sit/tags");

  // Create the tag file path
  char tag_file_path[MAX_FILENAME_LENGTH];
  snprintf(tag_file_path, sizeof(tag_file_path), "%s/%s.txt", tags_folder_path,
           tag_name);

  // Check if the tag file exists
  if (access(tag_file_path, F_OK) == 0) {
    FILE *tag_file = fopen(tag_file_path, "r");
    if (tag_file != NULL) {
      char tag_info[MAX_FILENAME_LENGTH];
      while (fgets(tag_info, sizeof(tag_info), tag_file) != NULL) {
        printf("%s", tag_info); // Display tag information
      }
      fclose(tag_file);
    } else {
      fprintf(stderr, "Error opening tag file.\n");
    }
  } else {
    fprintf(stderr, "Tag '%s' not found.\n", tag_name);
  }
}
void print_colored_line(const char *line, const char *color) {
  printf("%s%s%s", color, line, ANSI_COLOR_RESET);
}
int run_diff(const char *file1, const char *file2, int line1_start,
             int line1_end, int line2_start, int line2_end) {
  FILE *fp1, *fp2;
  char line1[1024], line2[1024];
  fp1 = fopen(file1, "r");
  fp2 = fopen(file2, "r");
  if (fp1 == NULL || fp2 == NULL) {
    fprintf(stderr, "Error: Unable to open files.\n");
    return 1;
  }

  // Read and compare lines
  int line_number = 0;

  // Skip lines until we reach line1_start
  while (line_number < line1_start - 1 &&
         fgets(line1, sizeof(line1), fp1) != NULL)
    line_number++;

  int line_number2 = 0;

  // Skip lines until we reach line2_start
  while (line_number2 < line2_start - 1 &&
         fgets(line2, sizeof(line2), fp2) != NULL)
    line_number2++;

  while (line_number <= line1_end && line_number2 <= line2_end &&
         fgets(line1, sizeof(line1), fp1) != NULL &&
         fgets(line2, sizeof(line2), fp2) != NULL) {
    line_number++;
    line_number2++;

    // Skip empty lines in file 1
    while ((line_number <= line1_end) &&
           (strlen(line1) == 0 || strspn(line1, " \t\n") == strlen(line1))) {
      if (fgets(line1, sizeof(line1), fp1) == NULL)
        break;
      line_number++;
    }

    // Skip empty lines in file 2
    while ((line_number2 <= line2_end) &&
           (strlen(line2) == 0 || strspn(line2, " \t\n") == strlen(line2))) {
      if (fgets(line2, sizeof(line2), fp2) == NULL)
        break;
      line_number2++;
    }

    // Compare non-empty lines
    if (strcmp(line1, line2) != 0) {
      printf("Difference found!\n");
      printf("%s-%d\n", file1, line_number);
      print_colored_line(line1, ANSI_COLOR_RED);
      printf("%s-%d\n", file2, line_number2);
      print_colored_line(line2, ANSI_COLOR_GREEN);
    }
  }

  // Close files
  fclose(fp1);
  fclose(fp2);

  return 0;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stdout, "please enter a valid command.");
    return 1;
  }
  print_command(argc, argv);

  if (strcmp(argv[1], "init") == 0) {
    return run_init(argc, argv);
  } else if (strcmp(argv[1], "add") == 0) {
    return run_add(argc, argv);
  } else if (strcmp(argv[1], "config") == 0) {
    return create_configs(argc, argv);
  } else if (strcmp(argv[1], "reset") == 0) {
    return run_reset(argc, argv);
  } else if (strcmp(argv[1], "set") == 0) {
    return run_command_with_shortcut(argc, argv);
  } else if (strcmp(argv[1], "replace") == 0) {
    return run_replace_command(argc, argv);
  } else if (strcmp(argv[1], "remove") == 0) {
    return run_remove_command(argc, argv);
  } else if (strcmp(argv[1], "commit") == 0) {
    return run_commit(argc, argv);
  } else if (strcmp(argv[1], "log") == 0) {
    if (argc == 5 &&
        (strcmp(argv[2], "-since") == 0 || strcmp(argv[2], "-before") == 0)) {
      const char *time_option = argv[2];
      const char *user_time = argv[3];
      filter_log_by_time_range(".sit/commit_log.txt", time_option, user_time);
      return 0;
    }
    if (argc > 3 && strcmp(argv[1], "log") == 0 &&
        strcmp(argv[2], "-search") == 0) {
      // Execute the search_commits_by_words function
      search_commits_by_words(argc, argv);
    } else {
      run_log(argc, argv);
      return 0;
    }
  } else if (strcmp(argv[1], "branch") == 0) {
    return run_branch(argc, argv);
  } else if (strcmp(argv[1], "checkout") == 0) {
    if (argc != 3) {
      fprintf(stderr, "Usage: %s checkout <commit_id_or_branch>\n", argv[0]);
      return 1;
    }
    const char *target = argv[2];
    return run_checkout(target);
  } else if (strcmp(argv[1], "status") == 0) {
    return run_status();
  } else if (strcmp(argv[1], "revert") == 0) {
    if (strstr(argv[2], "HEAD") != NULL || strstr(argv[4], "HEAD") != NULL) {
      return run_revert2(argc, argv);
    } else {
      return run_revert(argc, argv);
    }
  } else if (strcmp(argv[1], "tag") == 0) {
    if (argc == 2) {
      // If only "tag" is provided, list existing tags
      list_tags();
    } else {
      // Check if the user wants to view information about a specific tag
      if (argc == 3) {
        view_tag_info(argv[2]);
      } else {
        // Otherwise, create a new tag
        return run_tag(argc, argv);
      }
    }
  } else if (strcmp(argv[1], "diff") == 0) {
    if (argc < 6) {
      fprintf(stderr,
              "Usage: %s diff -f <file1> <file2> -line1 <begin-end> -line2 "
              "<begin-end>\n",
              argv[0]);
      return 1;
    }
    if (strcmp(argv[1], "diff") != 0 || strcmp(argv[2], "-f") != 0) {
      fprintf(stderr, "Invalid command. Use 'diff -f'.\n");
      return 1;
    }
    const char *file1 = argv[3];
    const char *file2 = argv[4];
    int line1_start = 0, line1_end = 0, line2_start = 0, line2_end = 0;

    for (int i = 5; i < argc; i++) {
      if (strcmp(argv[i], "-line1") == 0 && i + 1 < argc) {
        if (sscanf(argv[i + 1], "%d-%d", &line1_start, &line1_end) != 2) {
          line1_start = 0;
          FILE *files = fopen(file1, "r");
          char buffer[MAX_LINE_LENGTH];
          int count = 0;
          while (fgets(buffer, sizeof(buffer), files) != NULL) {
            count++;
          }
          line1_end = count;
          fclose(files);
          continue;
        }
        i++;
      } else if (strcmp(argv[i], "-line2") == 0 && i + 1 < argc) {
        // printf("meow") ;
        sscanf(argv[i + 1], "%d-%d", &line2_start, &line2_end);
        //  printf("%d" , line2_end ) ;
      } else {
        if (line2_end == 0) {
          line2_start = 0;
          FILE *file = fopen(file2, "r");
          char buffer[MAX_LINE_LENGTH];
          int count = 0;
          while (fgets(buffer, sizeof(buffer), file) != NULL) {
            count++;
          }
          line2_end = count;
          fclose(file);
          break;
        }
      }
    }
    // printf("%d %d %d %d", line1_start, line1_end, line2_start, line2_end);
    return run_diff(file1, file2, line1_start, line1_end, line2_start,
                    line2_end);
  } else if (argc > 1) {
    return run_command(argv[1]);
  }
}