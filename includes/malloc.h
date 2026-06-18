#ifndef MALLOC_H
# define MALLOC_H

# include <stddef.h> // instead of <stdlib.h> for size_t or there would be conflicts with the system malloc/free/realloc
# include <sys/mman.h>
# include <unistd.h>

# define TINY_MAX   128
# define SMALL_MAX  1024

# define ALIGN16(n) (((n) + 15) & ~(size_t)15)

typedef struct s_block
{
	struct s_block	*prev;
	struct s_block	*next;
	size_t			size;
	size_t			free;
}	t_block;

typedef struct s_zone
{
	struct s_zone	*prev;
	struct s_zone	*next;
	size_t			size;
	size_t			free;
}	t_zone;

typedef struct s_heap
{
	t_zone	*tiny;
	t_zone	*small;
	t_zone	*large;
}	t_heap;

extern t_heap	g_heap;


/*
	--------------------------------- Library API ---------------------------------
*/

// malloc.c
void	*malloc(size_t size);

// free.c
void	free(void *ptr);

// realloc.c
void	*realloc(void *ptr, size_t size);

// show_alloc.c
void	show_alloc_mem(void);


/*
	--------------------------------- Internal utilities ---------------------------
*/

// zone.c
t_zone	*new_zone(size_t zone_size, size_t max_block_size);
void	free_zone(t_zone **list, t_zone *zone);
t_zone	*find_zone(void *ptr, t_zone ***out_list);	
// find_zone takes a t_zone ***out_list so free.c gets back a pointer to the right list head (&g_heap.tiny, &g_heap.small, or &g_heap.large), enabling O(1) unlinking without a second traversal.

// utils.c
size_t	get_page_size(void);
size_t	get_zone_size(size_t max_block_size);
void	write_hex(unsigned long n);
void	write_size(size_t n);

#endif
