/*
 * Nerios_store_t : data type to handle memory
 */
#ifndef store_h
#define store_h

#include <stdlib.h>
#include <assert.h>
#include <vector>
#include <iostream>
#include <iomanip>

#define MEMMORY_WIDTH        40
#define MEMPAGE_BITS         16
#define MEMPAGE_SIZE         (1 << MEMPAGE_BITS)

#define NUM_PAGES            (1 << (MEMMORY_WIDTH-MEMPAGE_BITS))
#define PAGE_NUM(a)          ((a) >> MEMPAGE_BITS)
#define IS_VALID_PAGE(p_index)     (NULL != pages[p_index])
#define IS_PAGE_VISITED(p_index)   (true == is_page_visited[(p_index)])
#define PAGE_VISITED(p_index)      (is_page_visited[(p_index)] = true);
#define DATA_INDEX(a)        ((a) & (MEMPAGE_SIZE-1))
#define INIT_MEM_PAGES

class page_t_t
{
	uint8_t data[MEMPAGE_SIZE];

public:
	page_t_t(uint8_t default_val)
	{
#ifdef INIT_MEM_PAGES
		for (int i = 0; i < MEMPAGE_SIZE; i++)
			data[i] = default_val;
#endif
	}

	uint8_t& operator [](uint64_t a)
	{
		return data[a];
	}
};

#define FETCH_LINE_SIZE 32

class Nerios_store_t
{
	page_t_t* pages[NUM_PAGES]; // the actual store
	page_t_t* default_page;
	bool is_page_visited[NUM_PAGES]; // Used to track the valid pages
	uint8_t fetch_line_buffer[FETCH_LINE_SIZE];// an 8 word buffer maintained specially for optimization in icache accesses
	// if they cross page boundaries.
	uint8_t default_val;
	uint64_t max_page_limit;
	uint64_t num_pages_allocated;
	bool log_mem_usage_error;

	bool write_op_locate_page(uint64_t addr, page_t_t** p);
	bool read_op_locate_page(uint64_t addr, page_t_t** p);
	void track_page_allocation()
	{
		num_pages_allocated++;

	}

	void log_error_message()
	{

	}
public:

	Nerios_store_t();
	~Nerios_store_t();
	void reset(); // reset the mem
	void set_default_value(uint8_t value)
	{
		default_val = value;
	}

	void log_mem_usage_errors();

	void read_ptr(uint64_t addr, uint32_t len, uint8_t **);

	// access functions with overrun checks
	void write(uint64_t addr, uint32_t len, uint8_t data[]);
	void write(uint64_t addr, uint32_t len, uint8_t data[],std::vector<bool> mask);
	void read(uint64_t addr, uint32_t len, uint8_t data[]);

	// access functions with no overrun checks
	void write_nc(uint64_t addr, uint32_t len, uint8_t data[]);
	void read_nc(uint64_t addr, uint32_t len, uint8_t data[]);

	void dump_memory_contents()
	{
		page_t_t *p;
		for (uint64_t i=0;i<NUM_PAGES;i++)
		{
			if (is_page_visited[i] && (pages[i] != NULL))
			{
				std::cout << " PAGE: " << i << std::endl;
				p=pages[i];
				std::cout << "[" << std::hex << (i*MEMPAGE_SIZE) << "]: ";
				for (uint64_t j = 0; j < MEMPAGE_SIZE; j++)
				{
					if (((i*MEMPAGE_SIZE+j) % 64 == 0) && (j != 0))
						std::cout << std::endl << "[" << (i*MEMPAGE_SIZE+j) << "]: ";
					uint32_t value=uint32_t((*p)[j]);
					std::cout << std::setw(2) << value ;
				}
				std::cout << std::dec << std::endl;
			}
		}
	}
};

inline bool Nerios_store_t::read_op_locate_page(uint64_t addr, page_t_t** p)
{
	int page_index = PAGE_NUM(addr);

	if (!IS_VALID_PAGE(page_index))
	{

		//! This is the case of a UMR access in the application.
		//! In such a case, dont allocate memory. Return the
		//! default_page.


		*p = default_page;

		if (!IS_PAGE_VISITED(page_index))
		{

			//! The Page is being visited first. Logically,
			//! a new page is being allocated (not physically).
			//! Call the  MEM USAGE ERROR" function

			if (log_mem_usage_error)
			{
				std::cout << "WARNING: Page fault in simulated program at physical address 0x" << std::hex << addr << std::dec << std::endl;
				track_page_allocation();
			}

		}

		PAGE_VISITED(page_index);

	}
	else
	{

		*p = pages[page_index];
		assert(*p);

	}

	return true;

}
inline bool Nerios_store_t::write_op_locate_page(uint64_t addr, page_t_t** p)
{
	int page_index = PAGE_NUM(addr);

	if (!IS_VALID_PAGE(page_index))
	{

		//! not a valid page. So, allocate it
		*p = pages[page_index] = new page_t_t(default_val);
		assert(*p);

		PAGE_VISITED(page_index);

		if (log_mem_usage_error)
		{
			track_page_allocation();
		}

	}
	else
	{

		*p = pages[page_index];
		assert(*p);

	}

	return true;
}

inline void Nerios_store_t::write(uint64_t addr, uint32_t len, uint8_t data[])
{
	page_t_t *p;

	if (!write_op_locate_page(addr, &p))
		return;

	uint64_t index = DATA_INDEX(addr);

	if (index + len > MEMPAGE_SIZE)
	{
		uint64_t this_page = MEMPAGE_SIZE - index;
		write(addr + this_page, len - this_page, &data[this_page]);
		len = this_page;
	}

	for (uint64_t i = 0; i < len; i++)
	{
		(*p)[index + i] = data[i];
	}
}

inline void Nerios_store_t::write(uint64_t addr, uint32_t len, uint8_t data[],std::vector<bool> mask)
{
	page_t_t *p;

	if (!write_op_locate_page(addr, &p))
		return;

	uint64_t index = DATA_INDEX(addr);

	if (index + len > MEMPAGE_SIZE)
	{
		uint64_t this_page = MEMPAGE_SIZE - index;
		write(addr + this_page, len - this_page, &data[this_page], std::vector<bool>(mask.begin()+this_page,mask.end()));
		len = this_page;
	}

	for (uint64_t i = 0; i < len; i++)
	{
		if (mask[i])
			(*p)[index + i] = data[i];
	}

}

inline void Nerios_store_t::read_ptr(uint64_t addr, uint32_t len, uint8_t ** mem_ptr)
{
	page_t_t *p;

	assert(len<=FETCH_LINE_SIZE);

	if (!read_op_locate_page(addr, &p))
		return;

	uint64_t index = DATA_INDEX(addr);

	if (index + len > MEMPAGE_SIZE)
	{
		uint64_t this_page = MEMPAGE_SIZE - index;
		read_nc(addr + this_page, len - this_page, &fetch_line_buffer[this_page]);
		len = this_page;
		for (uint64_t i = 0; i < len; i++)
		{
			fetch_line_buffer[i] = (*p)[index + i];
		}
		*mem_ptr = fetch_line_buffer;
	}
	else
		*mem_ptr = &((*p)[index]);

}

inline void Nerios_store_t::read(uint64_t addr, uint32_t len, uint8_t data[])
{
	page_t_t *p;

	if (!read_op_locate_page(addr, &p))
		return;

	uint64_t index = DATA_INDEX(addr);

	if (index + len > MEMPAGE_SIZE)
	{
		uint64_t this_page = MEMPAGE_SIZE - index;
		read(addr + this_page, len - this_page, &data[this_page]);
		len = this_page;
	}

	for (uint64_t i = 0; i < len; i++)
	{
		data[i] = (*p)[index + i];
	}
}

inline void Nerios_store_t::write_nc(uint64_t addr, uint32_t len, uint8_t data[])
{
	page_t_t *p;

	if (!write_op_locate_page(addr, &p))
		return;

	uint64_t index = DATA_INDEX(addr);

	/* assume the access lies within this page - reasonable for
	 aligned acceses or of size 1*/

	for (uint64_t i = 0; i < len; i++)
	{
		(*p)[index + i] = data[i];
	}
}

inline void Nerios_store_t::read_nc(uint64_t addr, uint32_t len, uint8_t data[])
{
	page_t_t *p;

	if (!read_op_locate_page(addr, &p))
		return;

	uint64_t index = DATA_INDEX(addr);

	/* assume the access lies within this page - reasonable for
	 aligned acceses or of size 1*/

	for (uint64_t i = 0; i < len; i++)
	{
		data[i] = (*p)[index + i];
	}
}


const unsigned long BYTES_PER_MB =  0x100000;
const unsigned long DEFAULT_MAX_PAGE_LIMIT = 0x400;

Nerios_store_t::Nerios_store_t()
  :log_mem_usage_error(false),
   max_page_limit(DEFAULT_MAX_PAGE_LIMIT), // default 64 MB === 1K pages of 64KB size
   num_pages_allocated(0)
{


  default_val = 0;//xDE;
  default_page = new page_t_t(default_val);
  int p = 0;
  for(p=0; p < NUM_PAGES; p++){
    pages[p] = NULL;
    is_page_visited[p] = false;
  }
  for(p=0;p<32;p++)
    fetch_line_buffer[p]=0;

//  set_default_value(0xFF);
}

void Nerios_store_t::log_mem_usage_errors()
{
  log_mem_usage_error = true;
}


Nerios_store_t::~Nerios_store_t()
{
  reset();
  delete default_page;
}

void Nerios_store_t::reset()
{
  for(int p=0; p < NUM_PAGES; p++){
    if( pages[p] != NULL ){
      delete pages[p];
      pages[p] = NULL;
      is_page_visited[p] = false;
    }
  }
   num_pages_allocated = 0;
}


#endif
