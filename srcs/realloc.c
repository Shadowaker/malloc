#include "malloc.h"
#include "libft.h"

void	*realloc(void *ptr, size_t size)
{
	t_block	*block;
	size_t	old_size;
	size_t	aligned;
	void	*new_ptr;

	if (!ptr)
		return (malloc(size));

	if (size == 0)
	{
		free(ptr);
		return (NULL);
	}

	block = (t_block *)ptr - 1;
	old_size = block->size;
	aligned = ALIGN16(size);

	if (aligned == old_size)
		return (ptr);

	if (aligned < old_size)
	{
		if ((old_size <= TINY_MAX && aligned <= TINY_MAX) || (old_size <= SMALL_MAX && aligned <= SMALL_MAX))
			return (ptr);
	}

	new_ptr = malloc(size);

	if (!new_ptr)
		return (NULL);

	ft_memcpy(new_ptr, ptr, old_size < aligned ? old_size : aligned);
	free(ptr);

	return (new_ptr);
}
