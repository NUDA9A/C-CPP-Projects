//
// Created by vlaki on 09.06.2024.
//

#ifndef CT_C24_LW_CONTAINERS_NUDA9A_BUCKET_STORAGE_HPP
#define CT_C24_LW_CONTAINERS_NUDA9A_BUCKET_STORAGE_HPP

#include "bucket_iterator.hpp"
#include "my_stack.hpp"
#include "structs.hpp"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>

template< typename T >
class BucketStorage
{
  public:
	using size_type = std::size_t;
	using value_type = T;
	using pointer = T*;
	using reference = T&;
	using difference_type = std::ptrdiff_t;
	using const_reference = const T&;

	using iterator = Iterator< value_type >;
	using const_iterator = ConstIterator< value_type >;

	BucketStorage(const BucketStorage& other);
	BucketStorage(BucketStorage&& other) noexcept;
	explicit BucketStorage(size_type block_capacity = 64);
	BucketStorage& operator=(const BucketStorage& other);
	BucketStorage& operator=(BucketStorage&& other) noexcept;
	~BucketStorage();

  private:
	size_type current_size;
	size_type block_capacity;
	size_type current_capacity;
	size_type id_node;
	size_type id_block;
	MyStack< Node< value_type > > deleted_nodes;
	MyStack< Block< value_type > > deleted_blocks;
	Block< value_type >* head;
	Block< value_type >* tail;

	void copy(const BucketStorage& other);
	void move(BucketStorage&& other) noexcept;
	iterator get_position(Node< value_type >* node);
	iterator insert_impl(Node< value_type >* node);

	template< typename >
	friend class ConstIterator;

	template< typename >
	friend class Iterator;

	template< typename >
	friend class Node;

  public:
	iterator end() noexcept { return iterator(nullptr, tail); }
	iterator begin() noexcept { return iterator(head ? head->b_head : nullptr, head); }
	const_iterator end() const noexcept { return const_iterator(nullptr, tail); }
	const_iterator begin() const noexcept { return const_iterator(head ? head->b_head : nullptr, head); }
	const_iterator cend() const noexcept { return const_iterator(nullptr, tail); }
	const_iterator cbegin() const noexcept { return const_iterator(head ? head->b_head : nullptr, head); }

	iterator insert(const value_type& value);
	iterator insert(value_type&& value);
	iterator erase(const_iterator it);
	[[nodiscard]] bool empty() const noexcept;
	[[nodiscard]] size_type size() const noexcept;
	[[nodiscard]] size_type capacity() const noexcept;
	void swap(BucketStorage& other) noexcept;
	void clear();
	iterator get_to_distance(iterator it, difference_type distance);
	void shrink_to_fit();
};

template< typename T >
typename BucketStorage< T >::iterator BucketStorage< T >::insert_impl(Node< value_type >* node)
{
	iterator res_it = get_position(node);
	Block< value_type >* res_block = res_it.current_block;
	current_size++;

	node->node_id = node->node_id == 0 ? ++id_node : res_it.current_node->node_id;

	node->is_active = true;
	node->block = res_block;

	iterator it(node, res_block);

	return it;
}

template< typename T >
typename BucketStorage< T >::iterator BucketStorage< T >::get_position(Node< value_type >* node)
{
	try
	{
		Block< value_type >* res_block;

		if (deleted_nodes.size() != 0)
		{
			Node< value_type >* d_node = deleted_nodes.pop();
			node->prev = d_node->prev;
			node->next = d_node->next;
			node->block = d_node->block;
			node->node_id = d_node->node_id;
			node->is_active = true;
			if (node->prev)
			{
				node->prev->next = node;
			}
			if (node->next)
			{
				node->next->prev = node;
			}
			if (node->block->block_id == deleted_blocks.last()->block_id)
			{
				res_block = deleted_blocks.pop();
				current_capacity += block_capacity;
				res_block->is_active = true;
			}
			else
			{
				res_block = d_node->block;
			}
		}
		else
		{
			if (tail->prev && tail->prev->block_size < block_capacity)
			{
				res_block = tail->prev;
			}
			else
			{
				res_block = new Block< value_type >(++id_block);
				current_capacity += block_capacity;
				if (tail->prev == nullptr)
				{
					head = res_block;
				}
				res_block->next = tail;
				res_block->prev = tail->prev;
				tail->prev = res_block;
			}

			if (res_block->b_tail)
			{
				node->prev = res_block->b_tail;
				res_block->b_tail->next = node;
			}
			else
			{
				res_block->b_head = node;
			}
			res_block->b_tail = node;
		}

		if (res_block->next)
		{
			res_block->next->prev = res_block;
		}
		if (res_block->prev)
		{
			res_block->prev->next = res_block;
		}
		if (node->next)
		{
			node->next->prev = node;
		}
		if (node->prev)
		{
			node->prev->next = node;
		}

		node->block = res_block;
		res_block->block_size++;

		return iterator(node, res_block);
	} catch (std::bad_alloc& n)
	{
		std::cerr << "Error not enough memory: " << n.what() << std::endl;
		clear();
		throw n;
	}
}

template< typename T >
void BucketStorage< T >::shrink_to_fit()
{
	if (current_size == 0)
	{
		clear();
		return;
	}

	size_type new_size;

	size_type count = 1;

	while (current_size > block_capacity * count)
	{
		count++;
	}
	new_size = block_capacity * count;

	BucketStorage new_storage(new_size);
	iterator it = begin();
	for (it; it != end(); ++it)
	{
		new_storage.insert(*it);
	}
	*this = std::move(new_storage);
}

template< typename T >
typename BucketStorage< T >::iterator
	BucketStorage< T >::get_to_distance(BucketStorage::iterator it, const BucketStorage::difference_type distance)
{
	iterator result = it;
	if (distance > 0)
	{
		for (difference_type i = 0; i < distance; i++)
		{
			++result;
		}
	}
	else if (distance < 0)
	{
		for (difference_type i = distance; i < 0; i++)
		{
			--result;
		}
	}

	return result;
}

template< typename T >
void BucketStorage< T >::swap(BucketStorage& other) noexcept
{
	std::swap(current_size, other.current_size);
	std::swap(block_capacity, other.block_capacity);
	std::swap(current_capacity, other.current_capacity);
	std::swap(id_block, other.id_block);
	std::swap(id_node, other.id_node);
	std::swap(head, other.head);
	std::swap(tail, other.tail);
	deleted_blocks.swap(other.deleted_blocks);
	deleted_nodes.swap(other.deleted_nodes);
}

template< typename T >
typename BucketStorage< T >::size_type BucketStorage< T >::capacity() const noexcept
{
	return current_capacity;
}

template< typename T >
typename BucketStorage< T >::size_type BucketStorage< T >::size() const noexcept
{
	return current_size;
}

template< typename T >
bool BucketStorage< T >::empty() const noexcept
{
	return current_size == 0;
}

template< typename T >
typename BucketStorage< T >::iterator BucketStorage< T >::erase(BucketStorage::const_iterator it)
{
	if (current_size - 1 == 0)
	{
		clear();
		return end();
	}
	current_size--;

	Block< value_type >* current_block = it.current_block;
	current_block->block_size--;
	Node< value_type >* current_node = it.current_node;
	current_node->is_active = false;
	delete current_node->value_ptr;
	current_node->value_ptr = nullptr;
	deleted_nodes.push(current_node);
	if (current_node->prev)
	{
		if (current_node->next)
		{
			current_node->prev->next = current_node->next;
			current_node->next->prev = current_node->prev;
		}
		else
		{
			current_node->prev->next = nullptr;
			current_block->b_tail = current_node->prev;
		}
	}
	else
	{
		if (current_node->next)
		{
			current_node->next->prev = nullptr;
			current_block->b_head = current_node->next;
		}
		else
		{
			current_block->b_head = nullptr;
			current_block->b_tail = nullptr;
			current_block->is_active = false;
			deleted_blocks.push(current_block);
			current_capacity -= block_capacity;
			if (current_block->prev)
			{
				current_block->prev->next = current_block->next;
				current_block->next->prev = current_block->prev;
			}
			else
			{
				current_block->next->prev = nullptr;
				head = current_block->next;
			}
		}
	}

	++it;

	return iterator(it.current_node, it.current_block);
}

template< typename T >
typename BucketStorage< T >::iterator BucketStorage< T >::insert(value_type&& value)
{
	try
	{
		auto* res_node = new Node< value_type >();
		res_node->value_ptr = new value_type(std::move(value));
		return insert_impl(res_node);
	} catch (std::bad_alloc& n)
	{
		std::cerr << "Error not enough memory: " << n.what() << std::endl;
		clear();
		throw n;
	}
}

template< typename T >
typename BucketStorage< T >::iterator BucketStorage< T >::insert(const value_type& value)
{
	try
	{
		auto* res_node = new Node< value_type >();
		res_node->value_ptr = new value_type(value);
		return insert_impl(res_node);
	} catch (std::bad_alloc& n)
	{
		std::cerr << "Error not enough memory: " << n.what() << std::endl;
		clear();
		throw n;
	}
}

template< typename T >
void BucketStorage< T >::copy(const BucketStorage& other)
{
	try
	{
		head = nullptr;
		tail = new Block< value_type >();
		current_size = other.current_size;
		current_capacity = other.current_capacity;
		block_capacity = other.block_capacity;
		id_block = other.id_block;
		id_node = other.id_node;

		if (other.head == nullptr)
		{
			return;
		}

		Block< value_type >* other_block = other.head;
		auto* block = new Block< value_type >(other_block->block_id);
		block->block_size = other_block->block_size;
		head = block;
		while (other_block)
		{
			Node< value_type >* other_node = other_block->b_head;
			auto new_value_ptr = new value_type(*(other_node->value_ptr));
			auto* node = new Node< value_type >(new_value_ptr, other_node->node_id, block);
			block->b_head = node;

			for (size_type i = 1; i < block->block_size; i++)
			{
				new_value_ptr = new value_type(*(other_node->next->value_ptr));
				node->next = new Node< value_type >(new_value_ptr, other_node->next->node_id, block);
				node->next->prev = node;
				node = node->next;
				other_node = other_node->next;
			}

			block->b_tail = node;

			if (other_block->next->next)
			{
				block->next = new Block< value_type >(other_block->next->block_id);
				block->next->prev = block;
				block->next->block_size = other_block->next->block_size;
				block = block->next;
				other_block = other_block->next;
			}
			else
			{
				block->next = tail;
				tail->prev = block;
				other_block = other_block->next->next;
			}
		}

		size_type other_ptr = other.deleted_blocks.ptr;
		deleted_blocks.sz = other.deleted_blocks.sz;
		deleted_blocks.elements = new Block< value_type >*[deleted_blocks.sz];
		deleted_blocks.ptr = other_ptr;
		while (other_ptr > 0)
		{
			Block< value_type >* other_deleted_block = other.deleted_blocks.elements[--other_ptr];
			auto* d_block = new Block< value_type >(other_deleted_block->block_id);
			d_block->is_active = false;
			if (other_deleted_block->prev)
			{
				size_type d_block_id = other_deleted_block->prev->block_id;
				if (other_deleted_block->prev->is_active)
				{
					Block< value_type >* current_block = head;
					while (current_block)
					{
						if (current_block->block_id == d_block_id)
						{
							d_block->prev = current_block;
							break;
						}
						current_block = current_block->next;
					}
				}
				else
				{
					for (size_type i = deleted_blocks.ptr - 1; i > other_ptr; i--)
					{
						Block< value_type >* current_block = deleted_blocks.elements[i];
						if (current_block->block_id == d_block_id)
						{
							d_block->prev = current_block;
							break;
						}
					}
				}
			}
			else
			{
				d_block->prev = nullptr;
			}

			size_type d_block_id = other_deleted_block->next->block_id;
			if (other_deleted_block->next->is_active)
			{
				Block< value_type >* current_block = head;
				while (current_block)
				{
					if (current_block->block_id == d_block_id)
					{
						d_block->next = current_block;
						break;
					}
					current_block = current_block->next;
				}
			}
			else
			{
				if (other_deleted_block->next != other.tail)
				{
					for (size_type i = other.deleted_blocks.ptr - 1; i > other_ptr; i--)
					{
						Block< value_type >* current_block = deleted_blocks.elements[i];
						if (current_block->block_id == d_block_id)
						{
							d_block->next = current_block;
							break;
						}
					}
				}
				else
				{
					d_block->next = tail;
				}
			}
			deleted_blocks.elements[other_ptr] = d_block;
		}
		other_ptr = other.deleted_nodes.ptr;
		deleted_nodes.sz = other.deleted_nodes.sz;
		deleted_nodes.elements = new Node< value_type >*[deleted_nodes.sz];
		deleted_nodes.ptr = other_ptr;
		while (other_ptr > 0)
		{
			auto* d_node = new Node< value_type >();
			Node< value_type >* other_deleted_node = other.deleted_nodes.elements[--other_ptr];
			size_type d_block_id = other_deleted_node->block->block_id;
			Block< value_type >* block_for_node = nullptr;
			d_node->node_id = other_deleted_node->node_id;
			bool active = other_deleted_node->block->is_active;
			if (active)
			{
				Block< value_type >* current_block = head;
				while (current_block)
				{
					if (current_block->block_id == d_block_id)
					{
						block_for_node = current_block;
						break;
					}
					current_block = current_block->next;
				}
			}
			else
			{
				for (size_type i = 0; i < deleted_blocks.ptr; i++)
				{
					Block< value_type >* current_block = deleted_blocks.elements[i];
					if (current_block->block_id == d_block_id)
					{
						block_for_node = current_block;
						break;
					}
				}
			}
			d_node->block = block_for_node;

			if (other_deleted_node->prev)
			{
				size_type d_node_id = other_deleted_node->prev->node_id;
				if (other_deleted_node->prev->is_active)
				{
					Node< value_type >* current_node = block_for_node->b_head;
					while (current_node)
					{
						if (current_node->node_id == d_node_id)
						{
							d_node->prev = current_node;
							break;
						}
						current_node = current_node->next;
					}
				}
				else
				{
					for (size_type i = deleted_nodes.ptr - 1; i > other_ptr; i--)
					{
						Node< value_type >* current_node = deleted_nodes.elements[i];
						if (current_node->node_id == d_node_id)
						{
							d_node->prev = current_node;
							break;
						}
					}
				}
			}
			else
			{
				d_node->prev = nullptr;
			}

			if (other_deleted_node->next)
			{
				size_type d_node_id = other_deleted_node->next->node_id;
				if (other_deleted_node->next->is_active)
				{
					Node< value_type >* current_node = block_for_node->b_head;
					while (current_node)
					{
						if (current_node->node_id == d_node_id)
						{
							d_node->next = current_node;
							break;
						}
						current_node = current_node->next;
					}
				}
				else
				{
					for (size_type i = deleted_nodes.ptr - 1; i > other_ptr; i--)
					{
						Node< value_type >* current_node = deleted_nodes.elements[i];
						if (current_node->node_id == d_node_id)
						{
							d_node->next = current_node;
							break;
						}
					}
				}
			}
			else
			{
				d_node->next = nullptr;
			}
			deleted_nodes.elements[other_ptr] = d_node;
		}
	} catch (std::bad_alloc& n)
	{
		std::cerr << "Error not enough memory: " << n.what() << std::endl;
		clear();
		throw n;
	}
}

template< typename T >
void BucketStorage< T >::clear()
{
	while (head)
	{
		Block< value_type >* b_next = head->next;
		delete head;
		head = b_next;
	}
	head = nullptr;

	current_size = 0;
	current_capacity = 0;
	id_node = 0;
	id_block = 0;
	deleted_blocks.clear();
	deleted_nodes.clear();
	tail = new Block< value_type >();
}

template< typename T >
BucketStorage< T >& BucketStorage< T >::operator=(BucketStorage&& other) noexcept
{
	if (this != &other)
	{
		clear();
		move(std::move(other));
	}
	return *this;
}

template< typename T >
BucketStorage< T >& BucketStorage< T >::operator=(const BucketStorage& other)
{
	if (this != &other)
	{
		clear();
		copy(other);
	}
	return *this;
}

template< typename T >
BucketStorage< T >::BucketStorage(const BucketStorage& other)
{
	tail = new Block< value_type >();
	current_size = other.current_size;
	block_capacity = other.block_capacity;
	current_capacity = other.current_capacity;
	id_block = 0;
	id_node = 0;
	head = nullptr;
	copy(other);
}

template< typename T >
void BucketStorage< T >::move(BucketStorage&& other) noexcept
{
	head = std::exchange(other.head, nullptr);
	tail = std::exchange(other.tail, nullptr);
	current_size = std::exchange(other.current_size, 0);
	block_capacity = std::exchange(other.block_capacity, 0);
	id_node = std::exchange(other.id_node, 0);
	id_block = std::exchange(other.id_block, 0);
	current_capacity = std::exchange(other.current_capacity, 0);
	deleted_blocks = std::move(other.deleted_blocks);
	deleted_nodes = std::move(other.deleted_nodes);
}

template< typename T >
BucketStorage< T >::BucketStorage(BucketStorage&& other) noexcept
{
	current_size = 0;
	block_capacity = 0;
	id_block = 0;
	id_node = 0;
	current_capacity = 0;
	move(std::move(other));
}

template< typename T >
BucketStorage< T >::~BucketStorage()
{
	clear();
	delete tail;
}

template< typename T >
BucketStorage< T >::BucketStorage(size_type capacity)
{
	tail = new Block< value_type >();
	id_block = 0;
	id_node = 0;
	current_size = 0;
	block_capacity = capacity;
	head = nullptr;
	current_capacity = 0;
}

#endif	  // CT_C24_LW_CONTAINERS_NUDA9A_BUCKET_STORAGE_HPP
