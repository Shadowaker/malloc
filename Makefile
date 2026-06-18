ifeq ($(HOSTTYPE),)
	HOSTTYPE := $(shell uname -m)_$(shell uname -s)
endif

NAME	= libft_malloc_$(HOSTTYPE).so
LINK	= libft_malloc.so

CC		= gcc
CFLAGS	= -Wall -Wextra -Werror -fPIC
INC		= -I includes -I libft

SRCS	= srcs/malloc.c \
		  srcs/free.c \
		  srcs/realloc.c \
		  srcs/show_alloc_mem.c \
		  srcs/zone.c \
		  srcs/utils.c

OBJS_DIR	= objs
OBJS		= $(patsubst srcs/%.c,$(OBJS_DIR)/%.o,$(SRCS))

LIBFT_DIR	= libft
LIBFT		= $(LIBFT_DIR)/libft.a

TESTS_DIR	= tests

all: $(NAME) $(LINK)

$(OBJS_DIR)/%.o: srcs/%.c | $(OBJS_DIR)
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

$(OBJS_DIR):
	mkdir -p $(OBJS_DIR)

$(NAME): $(OBJS) $(LIBFT)
	$(CC) -shared -o $@ $(OBJS) $(LIBFT)

$(LINK): $(NAME)
	ln -sf $(NAME) $(LINK)

$(LIBFT):
	$(MAKE) -C $(LIBFT_DIR) CFLAGS="-Wall -Wextra -Werror -fPIC"

clean:
	rm -rf $(OBJS_DIR)
	$(MAKE) -C $(LIBFT_DIR) clean

fclean: clean
	rm -f $(NAME) $(LINK)
	$(MAKE) -C $(LIBFT_DIR) fclean
	@($(MAKE) -C $(TESTS_DIR) clean) &> /dev/null

re: fclean all

.PHONY: all clean fclean re
