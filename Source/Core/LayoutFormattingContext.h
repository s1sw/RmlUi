/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef RMLUI_CORE_LAYOUTFORMATTINGCONTEXT_H
#define RMLUI_CORE_LAYOUTFORMATTINGCONTEXT_H

#include "../../Include/RmlUi/Core/Types.h"
#include "LayoutBlockBox.h"

namespace Rml {

class Box;

// TODO: Replace by e.g. virtual calls.
struct FormatSettings {
	const Box* override_initial_box = nullptr;
	Vector2f* out_visible_overflow_size = nullptr;
};

class FormattingContext {
public:
	enum class Type {
		Block,
		Inline,
		Table,
		Flex,
	};

	virtual void Format(Vector2f containing_block, FormatSettings format_settings) = 0;

protected:
	FormattingContext(Type type, FormattingContext* parent, Element* root_element) : type(type), parent(parent), root_element(root_element) {}

	Element* GetRootElement() { return root_element; }

	static void SubmitElementLayout(Element* element) { element->OnLayout(); }

private:
	Type type;
	FormattingContext* parent;
	Element* root_element;
};

class BlockFormattingContext final : public FormattingContext {
public:
	BlockFormattingContext(FormattingContext* parent, Element* element) : FormattingContext(Type::Block, parent, element) {}

	void Format(Vector2f containing_block, FormatSettings format_settings) override;

	float GetShrinkToFitWidth() const;

private:
	bool FormatBlockBox(BlockContainer* block_container, Element* element, FormatSettings format_settings = {});

	bool FormatBlockContainerChild(BlockContainer* block_container, Element* element);
	bool FormatInline(BlockContainer* block_context_box, Element* element);
	bool FormatInlineBlock(BlockContainer* block_context_box, Element* element);
	bool FormatFlex(BlockContainer* block_context_box, Element* element);
	bool FormatTable(BlockContainer* block_context_box, Element* element_table);

	UniquePtr<BlockContainer> containing_block_box; // TODO?
	BlockContainer* root_block_container = nullptr;
};

// Formats the contents for a root-level element (usually a document or floating element).
void FormatRoot(Element* element, Vector2f containing_block, FormatSettings format_settings = {});

} // namespace Rml
#endif
