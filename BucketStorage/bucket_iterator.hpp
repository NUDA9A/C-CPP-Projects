//
// Created by vlaki on 11.06.2024.
//

#ifndef CT_C24_LW_CONTAINERS_NUDA9A_BUCKET_ITERATOR_HPP
#define CT_C24_LW_CONTAINERS_NUDA9A_BUCKET_ITERATOR_HPP

#include <iterator>
#include <utility>

template< typename T >
class BucketStorage;

template< typename T >
class Node;

template< typename T >
class Block;

template< typename T >
class ConstIterator
{
  public:
	using iterator_category = std::bidirectional_iterator_tag;
	using value_type = T;
	using difference_type = std::ptrdiff_t;
	using pointer = const T*;
	using reference = const T&;

  private:
	template< typename >
	friend class BucketStorage;

	template< typename >
	friend class Iterator;

	ConstIterator() : current_block(nullptr), current_node(nullptr) {}

	ConstIterator(Node< value_type >* node, Block< value_type >* block)
	{
		current_node = node;
		current_block = block;
	}

  public:
	reference operator*() const { return *(current_node->value_ptr); }
	pointer operator->() const { return current_node->value_ptr; }

	ConstIterator& operator++()
	{
		if (current_node->next)
		{
			current_node = current_node->next;
		}
		else
		{
			if (current_block->next->next)
			{
				current_block = current_block->next;
				current_node = current_block->b_head;
			}
			else
			{
				current_block = current_block->next;
				current_node = nullptr;
			}
		}
		return *this;
	}

	ConstIterator operator++(int)
	{
		ConstIterator temp = *this;
		++(*this);
		return temp;
	}

	ConstIterator& operator--()
	{
		if (current_node)
		{
			if (current_node->prev)
			{
				current_node = current_node->prev;
			}
			else
			{
				if (current_block->prev)
				{
					current_block = current_block->prev;
					current_node = current_block->b_tail;
				}
				else
				{
					current_node = nullptr;
				}
			}
		}
		else
		{
			if (current_block->prev)
			{
				current_block = current_block->prev;
				current_node = current_block->b_tail;
			}
			else
			{
				current_node = nullptr;
			}
		}
		return *this;
	}

	ConstIterator operator--(int)
	{
		ConstIterator temp = *this;
		--(*this);
		return temp;
	}

	bool operator==(const ConstIterator& other) const { return current_node == other.current_node; }
	bool operator!=(const ConstIterator& other) const { return current_node != other.current_node; }

	ConstIterator& operator=(const ConstIterator& other)
	{
		if (this != &other)
		{
			current_node = other.current_node;
			current_block = other.current_block;
		}
		return *this;
	}

	ConstIterator(const ConstIterator& other)
	{
		current_node = other.current_node;
		current_block = other.current_block;
	}

	ConstIterator(ConstIterator&& other) noexcept
	{
		current_node = std::exchange(other.current_node, nullptr);
		current_block = std::exchange(other.current_block, nullptr);
	}

	ConstIterator& operator=(ConstIterator&& other) noexcept
	{
		if (this != &other)
		{
			current_node = std::exchange(other.current_node, nullptr);
			current_block = std::exchange(other.current_block, nullptr);
		}
		return *this;
	}

	bool operator>(const ConstIterator& other) const
	{
		if (other.current_block->block_id == 0)
		{
			return false;
		}
		if (current_block->block_id == 0)
		{
			return true;
		}
		return current_node->node_id > other.current_node->node_id;
	}

	bool operator<(const ConstIterator& other) const
	{
		if (other.current_block->block_id == 0)
		{
			if (current_block->block_id == 0)
			{
				return false;
			}
			return true;
		}
		return current_node->node_id < other.current_node->node_id;
	}

	bool operator>=(const ConstIterator& other) const { return (*this > other) || (*this == other); }

	bool operator<=(const ConstIterator& other) const { return (*this < other) || (*this == other); }

  protected:
	Block< value_type >* current_block;
	Node< value_type >* current_node;

	template< typename >
	friend class BucketStorage;
};

template< typename T >
class Iterator : public ConstIterator< T >
{
  public:
	using iterator_category = std::bidirectional_iterator_tag;
	using value_type = T;
	using difference_type = std::ptrdiff_t;
	using pointer = T*;
	using reference = T&;

  private:
	template< typename >
	friend class BucketStorage;

	Iterator() : ConstIterator< T >() {}
	Iterator(Node< value_type >* node, Block< value_type >* block) : ConstIterator< value_type >(node, block) {}

  public:
	reference operator*() { return *(this->current_node->value_ptr); }
	pointer operator->() { return this->current_node->value_ptr; }

	Iterator& operator++()
	{
		ConstIterator< T >::operator++();
		return *this;
	}

	Iterator operator++(int)
	{
		Iterator temp = *this;
		ConstIterator< T >::operator++();
		return temp;
	}

	Iterator& operator--()
	{
		ConstIterator< T >::operator--();
		return *this;
	}

	Iterator operator--(int)
	{
		Iterator temp = *this;
		ConstIterator< T >::operator--();
		return temp;
	}
};

#endif	  // CT_C24_LW_CONTAINERS_NUDA9A_BUCKET_ITERATOR_HPP
