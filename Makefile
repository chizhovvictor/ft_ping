NAME = ft_ping


SRCS =	./src/ft_ping.c \
		./src/utils.c 

HEADER = ./include/ft_ping.h

OBJS = $(SRCS:.c=.o)

CFLAGS = -Werror -Wextra -Wall -g -fsanitize=address

all: $(NAME)

CC = cc

%.o : %.c 
	$(CC) $(CFLAGS) -c $< -o $@

$(NAME) : $(OBJS)
	$(CC) -I $(HEADER) $(CFLAGS) $(OBJS) -o $(NAME)


clean:
	rm -rf $(OBJS)

fclean:
	rm -rf $(OBJS) $(NAME)

re: fclean all

.PHONY:	all clean fclean re bonus