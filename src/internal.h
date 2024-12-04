/*************************************************************************************************
 * MT3 1.1 - MoTree Library
 * A lightweight library for serializing and deserializing BTOs (Binary Tree Objects).
 * 
 * Copyright (c) 2024 Hidegi
 * 
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * 
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 * 
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
 *
 *************************************************************************************************/
#ifndef MT3_INTERNAL_IMPL_H
#define MT3_INTERNAL_IMPL_H
#include "mt3.h"
#include "platform.h"
#define MT3_CHECKED_CALLOC(ptr, n, size, on_error)		\
	do							\
	{							\
		if(!((ptr) = calloc(n, size)))			\
		{						\
			errno = MT3_STATUS_NO_MEMORY;		\
			on_error;				\
		}						\
	} while(0)

const char* _mt3_tag_to_str(MT3_tag tag);
MT3_node _mt3_search(const MT3_node tree, const char* name);
SPhash _mt3_sdbm(const SPchar* str);

SPbuffer _mt3_compress(const void* memory, SPsize length);
SPbuffer _mt3_decompress(const void* memory, SPsize length);
void _mt3_encode_tree(const MT3_node tree, SPbuffer* buffer, int level);
void _mt3_encode_list(const MT3_node list, SPbuffer* buffer, int level);
MT3_node _mt3_decode_tree(const SPubyte** memory, SPsize* length);
SPbool _mt3_decode_list(MT3_node node, const SPubyte** memory, SPsize* length);

SPbool _mt3_is_root(const MT3_node node);
SPbool _mt3_is_major(const MT3_node node);

void _mt3_fix_rbt_violations(MT3_node node, MT3_node* head);
void _mt3_delete_node(MT3_node n);
void _mt3_bst_delete_impl(MT3_node n, MT3_node* head, MT3_node* _r, MT3_node* _x, MT3_node* _w);
SPbool _mt3_fix_up_rbt(SPbool redBefore, MT3_node r, MT3_node x, MT3_node w, MT3_node* head);
void _mt3_print_tree(const MT3_node tree, SPbuffer* buffer, int level);
void _mt3_print_list(const MT3_node tree, SPbuffer* buffer, int level);
SPsize _mt3_length_of_list(const MT3_node list);
#endif