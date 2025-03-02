//
// Created by vlaki on 11.06.2024.
//

#ifndef CT_C24_LW_CONTAINERS_NUDA9A_MY_STACK_HPP
#define CT_C24_LW_CONTAINERS_NUDA9A_MY_STACK_HPP

#include <iostream>
#include <utility>
template< typename T >
class MyStack
{
  private:
	using size_type = std::size_t;
	using value_type = T;
	using pointer = T*;
	using reference = T&;

	MyStack() : ptr(0), elements(new pointer[10]), sz(10) {}

	~MyStack()
	{
		clear();
		delete[] elements;
	}

	void push(pointer value)
	{
		if (ptr == sz)
		{
			try
			{
				auto tmp = new pointer[sz * 2];
				std::copy(elements, elements + sz, tmp);
				delete[] elements;
				sz *= 2;
				elements = tmp;
			} catch (std::bad_alloc& n)
			{
				std::cerr << "Error not enough memory: " << n.what() << std::endl;
				clear();
				throw n;
			}
		}
		elements[ptr] = value;
		ptr++;
	}

	void clear()
	{
		for (size_type i = 0; i < ptr; i++)
		{
			delete elements[i];
			elements[i] = nullptr;
		}
		ptr = 0;
		sz = 0;
	}

	size_type size() { return ptr; }

	void swap(MyStack& other)
	{
		std::swap(ptr, other.ptr);
		std::swap(sz, other.sz);
		std::swap(elements, other.elements);
	}

	pointer pop()
	{
		ptr--;
		pointer result = elements[ptr];
		elements[ptr] = nullptr;
		return result;
	}

	pointer last() { return elements[ptr - 1]; }

	MyStack& operator=(MyStack&& other) noexcept
	{
		if (this != &other)
		{
			clear();

			move(std::move(other));
		}
		return *this;
	}

	MyStack(MyStack&& other) noexcept { move(std::move(other)); }

	void move(MyStack&& other) noexcept
	{
		elements = std::exchange(other.elements, nullptr);
		ptr = std::exchange(other.ptr, 0);
		sz = std::exchange(other.sz, 0);
	}

	size_type ptr;
	pointer* elements;
	size_type sz;

	template< typename >
	friend class BucketStorage;
};

#endif	  // CT_C24_LW_CONTAINERS_NUDA9A_MY_STACK_HPP
