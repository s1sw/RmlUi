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

#include "LayoutInlineBoxText.h"
#include "../../Include/RmlUi/Core/ComputedValues.h"
#include "../../Include/RmlUi/Core/Core.h"
#include "../../Include/RmlUi/Core/ElementText.h"
#include "../../Include/RmlUi/Core/ElementUtilities.h"
#include "../../Include/RmlUi/Core/FontEngineInterface.h"
#include "../../Include/RmlUi/Core/Log.h"
#include "../../Include/RmlUi/Core/Profiling.h"
#include "../../Include/RmlUi/Core/Property.h"
#include "ComputeProperty.h"
#include "LayoutEngine.h"
#include "LayoutLineBox.h"

namespace Rml {

String FontFaceDescription(const String& font_family, Style::FontStyle style, Style::FontWeight weight)
{
	String font_attributes;

	if (style == Style::FontStyle::Italic)
		font_attributes += "italic, ";
	if (weight == Style::FontWeight::Bold)
		font_attributes += "bold, ";
	else if (weight != Style::FontWeight::Auto && weight != Style::FontWeight::Normal)
		font_attributes += "weight=" + ToString((int)weight) + ", ";

	if (font_attributes.empty())
		font_attributes = "regular";
	else
		font_attributes.resize(font_attributes.size() - 2);

	return CreateString(font_attributes.size() + font_family.size() + 8, "'%s' [%s]", font_family.c_str(), font_attributes.c_str());
}

LayoutInlineBoxText::LayoutInlineBoxText(ElementText* element, int _line_begin) : LayoutInlineBox(static_cast<Element*>(element), Box())
{
	line_begin = _line_begin;

	// Build the box to represent the dimensions of the first word.
	BuildWordBox();
}

LayoutInlineBoxText::~LayoutInlineBoxText() {}

// Returns true if this box is capable of overflowing, or if it must be rendered on a single line.
bool LayoutInlineBoxText::CanOverflow() const
{
	return line_segmented;
}

// Flows the inline box's content into its parent line.
UniquePtr<LayoutInlineBox> LayoutInlineBoxText::FlowContent(bool first_box, float available_width, float right_spacing_width)
{
	ElementText* text_element = GetTextElement();
	RMLUI_ASSERT(text_element != nullptr);

	int line_length;
	float line_width;
	bool overflow =
		!text_element->GenerateLine(line_contents, line_length, line_width, line_begin, available_width, right_spacing_width, first_box, true);

	Vector2f content_area;
	content_area.x = line_width;
	content_area.y = GetBoxContentSize().y;
	SetBoxContentSize(content_area);

	// Call the base-class's FlowContent() to increment the width of our parent's box.
	LayoutInlineBox::FlowContent(first_box, available_width, right_spacing_width);

	if (overflow)
		return MakeUnique<LayoutInlineBoxText>(GetTextElement(), line_begin + line_length);

	return nullptr;
}

// Computes and sets the vertical position of this element, relative to its parent inline box (or block box, for an un-nested inline box).
void LayoutInlineBoxText::CalculateBaseline(float& ascender, float& descender)
{
	ascender = GetHeight() - GetBaseline();
	descender = GetHeight() - ascender;
}

// Offsets the baseline of this box, and all of its children, by the ascender of the parent line box.
void LayoutInlineBoxText::OffsetBaseline(float ascender)
{
	// Offset by the ascender.
	float increment = (ascender - (GetHeight() - GetBaseline()));

	// Calculate the leading (the difference between font height and line height).
	float leading = 0;

	FontFaceHandle font_face_handle = GetElement()->GetFontFaceHandle();
	if (font_face_handle != 0)
		leading = GetHeight() - GetFontEngineInterface()->GetLineHeight(font_face_handle);

	// Offset by the half-leading.
	increment += leading * 0.5f;

	SetVerticalPosition(GetPosition().y + increment);
}

void LayoutInlineBoxText::PositionElement(Vector2f relative_position, Element* offset_parent)
{
	auto text_element = GetTextElement();
	if (line_begin == 0)
	{
		LayoutInlineBox::PositionElement(relative_position, offset_parent);

		text_element->ClearLines();
		text_element->AddLine(Vector2f(0, 0), line_contents);
	}
	else
	{
		text_element->AddLine(relative_position + GetPosition() - text_element->GetRelativeOffset(Box::BORDER), line_contents);
	}
}

void LayoutInlineBoxText::SizeElement(bool /*split*/, Vector2f /*line_position*/) {}

String LayoutInlineBoxText::DumpNameValue() const
{
	return "LayoutInlineBoxText ('" + line_contents.substr(0, 20) + "')";
}

void* LayoutInlineBoxText::operator new(size_t size)
{
	return LayoutEngine::AllocateLayoutChunk(size);
}

void LayoutInlineBoxText::operator delete(void* chunk, size_t size)
{
	LayoutEngine::DeallocateLayoutChunk(chunk, size);
}

// Returns the box's element as a text element.
ElementText* LayoutInlineBoxText::GetTextElement()
{
	RMLUI_ASSERT(rmlui_dynamic_cast<ElementText*>(GetElement()));

	return static_cast<ElementText*>(GetElement());
}

// Builds a box for the first word of the element.
void LayoutInlineBoxText::BuildWordBox()
{
	RMLUI_ZoneScoped;

	ElementText* text_element = GetTextElement();
	RMLUI_ASSERT(text_element != nullptr);

	FontFaceHandle font_face_handle = text_element->GetFontFaceHandle();
	if (font_face_handle == 0)
	{
		SetHeight(0);
		SetBaseline(0);

		const ComputedValues& computed = text_element->GetComputedValues();
		const String font_family_property = computed.font_family();

		if (font_family_property.empty())
		{
			Log::Message(Log::LT_WARNING, "No font face defined. Missing 'font-family' property, please add it to your RCSS. On element %s",
				text_element->GetAddress().c_str());
		}
		else
		{
			const String font_face_description = FontFaceDescription(font_family_property, computed.font_style(), computed.font_weight());

			Log::Message(Log::LT_WARNING,
				"No font face defined. Ensure (1) that Context::Update is run after new elements are constructed, before Context::Render, "
				"and (2) that the specified font face %s has been successfully loaded. "
				"Please see previous log messages for all successfully loaded fonts. On element %s",
				font_face_description.c_str(), text_element->GetAddress().c_str());
		}

		return;
	}

	Vector2f content_area;
	line_segmented = !text_element->GenerateToken(content_area.x, line_begin);
	content_area.y = text_element->GetLineHeight();
	SetBoxContentSize(content_area);
}

} // namespace Rml
