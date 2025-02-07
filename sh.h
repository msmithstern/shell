ssize_t get_a_line(char buf[1024]);

void parse(char buffer[1024], char *tokens[512], char *argv[512]);

int check_command(char *tokens[512], char *str);

int execute_command(char *tokens[512]);

void files_handler(char *token, int file_no, int flags, int permissions);

void redirections(char *tokens[512], char *argv[512]);

char *get_filepath(char *tokens[512]);