YELLOW="\033[0;33m"
RED="\033[1;31m"
GREEN='\033[3;32m'
NONE='\033[0m'

NAME = ft_ping

CC = cc

CFLAGS = -Wall -Wextra -Werror

SRCS = $(wildcard *.c)

OBJS = $(patsubst %.c, %.o, $(SRCS))

RM = rm -rf


all: $(NAME)

%.o : %.c
	@echo $(NONE) $(YELLOW)"       Please wait ..." $(NONE)
	@$(CC) -s $(CFLAGS) -c $< -o $@

$(NAME): $(OBJS)
	@$(CC) -s $(OBJS) $(CFLAGS) -o $(NAME)
	@echo $(NONE) $(YELLOW)"       Please wait ..." $(NONE)
	@echo $(NONE) $(GREEN)"       Compiled $(NAME)" $(NONE)

clean:
	@echo $(NONE) $(RED)"       Removing object files" $(NONE)
	@echo $(NONE) $(YELLOW)"       Please wait ..." $(NONE)
	@$(RM) $(OBJS)

fclean: clean
	@echo $(NONE) $(RED)"       Removing $(NAME)" $(NONE)
	@echo $(NONE) $(YELLOW)"       Please wait ..." $(NONE)
	@$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re