NAME    = test_malloc

CC      = gcc
CFLAGS  = -Wall -Wextra -I ../includes
LDFLAGS = -L.. -lft_malloc -Wl,-rpath,..

SRCS    = test.c

all: $(NAME)

$(NAME): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

run: $(NAME)
	./$(NAME)

clean:
	rm -f $(NAME)

.PHONY: all run clean
