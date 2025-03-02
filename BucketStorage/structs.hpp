//
// Created by vlaki on 12.06.2024.
//

#ifndef CT_C24_LW_CONTAINERS_NUDA9A_STRUCTS_HPP
#define CT_C24_LW_CONTAINERS_NUDA9A_STRUCTS_HPP

template< typename T >
class BucketStorage;

template< typename T >
class Block;

template< typename T >
class Node
{
  private:
	using value_type = T;
	using size_type = std::size_t;
	using pointer = T*;

	template< typename >
	friend class ConstIterator;

	template< typename >
	friend class Iterator;

	template< typename >
	friend class Block;

	template< typename >
	friend class BucketStorage;

	template< typename >
	friend class MyStack;

	pointer value_ptr;
	size_type node_id;
	Node* next;
	Node* prev;
	Block< value_type >* block;
	bool is_active;

	Node() : block(nullptr), value_ptr(nullptr), next(nullptr), prev(nullptr), node_id(0), is_active(false) {}

	Node(pointer value_ptr, size_type id_node, Block< value_type >* b) :
		value_ptr(value_ptr), node_id(id_node), block(b), next(nullptr), prev(nullptr), is_active(true)
	{
	}

	~Node()
	{
		next = nullptr;
		prev = nullptr;
		block = nullptr;
		node_id = 0;
		is_active = false;
		delete value_ptr;
	}
};

template< typename T >
class Block
{
  private:
	using value_type = T;
	using size_type = std::size_t;

	template< typename >
	friend class ConstIterator;

	template< typename >
	friend class Iterator;

	template< typename >
	friend class BucketStorage;

	template< typename >
	friend class MyStack;

	Node< value_type >* b_head;
	Node< value_type >* b_tail;
	Block* next;
	Block* prev;
	size_type block_size;
	size_type block_id;
	bool is_active;

	explicit Block(size_type id_block) : block_id(id_block), next(nullptr), prev(nullptr)
	{
		b_head = nullptr;
		b_tail = nullptr;
		is_active = true;
		block_size = 0;
	}

	Block() :
		is_active(false), next(nullptr), prev(nullptr), block_id(0), block_size(0), b_head(nullptr), b_tail(nullptr)
	{
	}

	~Block()
	{
		next = nullptr;
		prev = nullptr;
		block_size = 0;
		block_id = 0;
		is_active = false;
		while (b_head)
		{
			Node< value_type >* next_node = b_head->next;
			delete b_head;
			b_head = next_node;
		}
	}
};

#endif	  // CT_C24_LW_CONTAINERS_NUDA9A_STRUCTS_HPP
