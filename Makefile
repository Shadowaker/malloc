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
TOTAL		= $(words $(OBJS))
BAR_WIDTH	= 30

LIBFT_DIR	= libft
LIBFT		= $(LIBFT_DIR)/libft.a

TESTS_DIR	= tests

GREEN	= \033[32m
RED		= \033[31m
RESET	= \033[0m
BOLD	= \033[1m

all: _welcome | $(NAME) $(LINK)
	@printf "\n$(GREEN)$(BOLD)Build successful: $(NAME)$(RESET)\n"

_welcome:
	@printf "$(BOLD)Welcome to malloc$(RESET)\n\n"

$(OBJS_DIR)/%.o: srcs/%.c | $(OBJS_DIR)
	@if ! OUTPUT=$$($(CC) $(CFLAGS) $(INC) -c $< -o $@ 2>&1); then \
		printf "\n$(RED)$$OUTPUT$(RESET)\n" >&2; exit 1; \
	fi
	@COUNT=$$(ls $(OBJS_DIR)/*.o 2>/dev/null | wc -l | tr -d ' '); \
	 FILLED=$$(($$COUNT * $(BAR_WIDTH) / $(TOTAL))); \
	 EMPTY=$$(($(BAR_WIDTH) - $$FILLED)); \
	 BAR=$$(printf '%*s' $$FILLED '' | tr ' ' '@'); \
	 EMP=$$(printf '%*s' $$EMPTY '' | tr ' ' '@'); \
	 printf "\r  Building [$(GREEN)%s$(RESET)%s] %d/%d" "$$BAR" "$$EMP" "$$COUNT" "$(TOTAL)"

$(OBJS_DIR):
	@mkdir -p $(OBJS_DIR)

$(NAME): $(OBJS) $(LIBFT)
	@$(CC) -shared -o $@ $(OBJS) $(LIBFT)

$(LINK): $(NAME)
	@ln -sf $(NAME) $(LINK)
	@$(MAKE) -s --no-print-directory -C $(TESTS_DIR)
	@printf "\n\n\r  Running tests...\n"
	@cd $(TESTS_DIR) && OUTPUT=$$(./test_malloc 2>&1); STATUS=$$?; \
	 printf "\r  %s\n" "$$OUTPUT" | tail -3; \
	 [ $$STATUS -eq 0 ] || printf "$(RED)$(BOLD)Warning: tests failed$(RESET)\n"
	@printf "\n"

$(LIBFT):
	@if ! OUTPUT=$$($(MAKE) -s --no-print-directory -C $(LIBFT_DIR) CFLAGS="-Wall -Wextra -Werror -fPIC" 2>&1); then \
		printf "$(RED)$$OUTPUT$(RESET)\n" >&2; exit 1; \
	fi

clean:
	@rm -rf $(OBJS_DIR)
	@$(MAKE) -s --no-print-directory -C $(LIBFT_DIR) clean

fclean: clean
	@rm -f $(NAME) $(LINK)
	@$(MAKE) -s --no-print-directory -C $(LIBFT_DIR) fclean
	@$(MAKE) -s --no-print-directory -C $(TESTS_DIR) clean >/dev/null 2>&1; true
	@printf "$(GREEN)$(BOLD)Full clean successful$(RESET)\n"

re: fclean all

.PHONY: all clean fclean re