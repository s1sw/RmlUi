/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
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

 
#include "precompiled.h"
#include "../../Include/Rocket/Core/StyleSheetSpecification.h"
#include "PropertyParserNumber.h"
#include "PropertyParserAnimation.h"
#include "PropertyParserColour.h"
#include "PropertyParserKeyword.h"
#include "PropertyParserString.h"
#include "PropertyParserTransform.h"
#include "PropertyShorthandDefinition.h"

namespace Rocket {
namespace Core {


static StyleSheetSpecification* instance = NULL;



StyleSheetSpecification::StyleSheetSpecification() : 
	// Reserve space for all defined ids and some more for custom properties
	properties(2 * (size_t)PropertyId::NumDefinedIds, 2 * (size_t)ShorthandId::NumDefinedIds)
{
	ROCKET_ASSERT(instance == NULL);
	instance = this;
}

StyleSheetSpecification::~StyleSheetSpecification()
{
	ROCKET_ASSERT(instance == this);
	instance = NULL;
}

PropertyDefinition& StyleSheetSpecification::RegisterProperty(PropertyId id, const String& property_name, const String& default_value, bool inherited, bool forces_layout)
{
	return properties.RegisterProperty(property_name, default_value, inherited, forces_layout, id);
}

bool StyleSheetSpecification::RegisterShorthand(ShorthandId id, const String& shorthand_name, const String& property_names, ShorthandType type)
{
	return properties.RegisterShorthand(shorthand_name, property_names, type, id);
}

bool StyleSheetSpecification::Initialise()
{
	if (instance == NULL)
	{
		new StyleSheetSpecification();

		instance->RegisterDefaultParsers();
		instance->RegisterDefaultProperties();
	}

	return true;
}

void StyleSheetSpecification::Shutdown()
{
	if (instance != NULL)
	{
		for (ParserMap::iterator iterator = instance->parsers.begin(); iterator != instance->parsers.end(); ++iterator)
			(*iterator).second->Release();

		delete instance;
	}
}

// Registers a parser for use in property definitions.
bool StyleSheetSpecification::RegisterParser(const String& parser_name, PropertyParser* parser)
{
	ParserMap::iterator iterator = instance->parsers.find(parser_name);
	if (iterator != instance->parsers.end())
		(*iterator).second->Release();

	instance->parsers[parser_name] = parser;
	return true;
}

// Returns the parser registered with a specific name.
PropertyParser* StyleSheetSpecification::GetParser(const String& parser_name)
{
	ParserMap::iterator iterator = instance->parsers.find(parser_name);
	if (iterator == instance->parsers.end())
		return NULL;

	return (*iterator).second;
}

// Registers a property with a new definition.
PropertyDefinition& StyleSheetSpecification::RegisterProperty(const String& property_name, const String& default_value, bool inherited, bool forces_layout)
{
	ROCKET_ASSERTMSG((size_t)instance->properties.property_map.GetId(property_name) < (size_t)PropertyId::FirstCustomId, "Custom property name matches an internal property, please make a unique name for the given property.");
	return instance->properties.RegisterProperty(property_name, default_value, inherited, forces_layout);
}

// Returns a property definition.
const PropertyDefinition* StyleSheetSpecification::GetProperty(const String& property_name)
{
	return instance->properties.GetProperty(property_name);
}

const PropertyDefinition* StyleSheetSpecification::GetProperty(PropertyId id)
{
	return instance->properties.GetProperty(id);
}

// Fetches a list of the names of all registered property definitions.
const PropertyNameList& StyleSheetSpecification::GetRegisteredProperties()
{
	return instance->properties.GetRegisteredProperties();
}

const PropertyNameList & StyleSheetSpecification::GetRegisteredInheritedProperties()
{
	return instance->properties.GetRegisteredInheritedProperties();
}

// Registers a shorthand property definition.
bool StyleSheetSpecification::RegisterShorthand(const String& shorthand_name, const String& property_names, ShorthandType type)
{
	ROCKET_ASSERTMSG(instance->properties.property_map.GetId(shorthand_name) == PropertyId::Invalid, "Custom shorthand name matches a property name, please make a unique name.");
	ROCKET_ASSERTMSG((size_t)instance->properties.shorthand_map.GetId(shorthand_name) < (size_t)ShorthandId::FirstCustomId, "Custom shorthand name matches an internal shorthand, please make a unique name for the given shorthand property.");
	return instance->properties.RegisterShorthand(shorthand_name, property_names, type);
}

// Returns a shorthand definition.
const ShorthandDefinition* StyleSheetSpecification::GetShorthand(const String& shorthand_name)
{
	return instance->properties.GetShorthand(shorthand_name);
}

const ShorthandDefinition* StyleSheetSpecification::GetShorthand(ShorthandId id)
{
	return instance->properties.GetShorthand(id);
}

// Parses a property declaration, setting any parsed and validated properties on the given dictionary.
bool StyleSheetSpecification::ParsePropertyDeclaration(PropertyDictionary& dictionary, const String& property_name, const String& property_value, const String& source_file, int source_line_number)
{
	// Try as a property first
	PropertyId property_id = GetPropertyId(property_name);
	if(property_id != PropertyId::Invalid)
		return instance->properties.ParsePropertyDeclaration(dictionary, property_id, property_value, source_file, source_line_number);

	// Then, as a shorthand
	ShorthandId shorthand_id = GetShorthandId(property_name);
	if(shorthand_id != ShorthandId::Invalid)
		return instance->properties.ParseShorthandDeclaration(dictionary, shorthand_id, property_value, source_file, source_line_number);

	return false;
}

PropertyId StyleSheetSpecification::GetPropertyId(const String& property_name)
{
	return instance->properties.property_map.GetId(property_name);
}

ShorthandId StyleSheetSpecification::GetShorthandId(const String& shorthand_name)
{
	return instance->properties.shorthand_map.GetId(shorthand_name);
}

const String& StyleSheetSpecification::GetPropertyName(PropertyId id)
{
	return instance->properties.property_map.GetName(id);
}

const String& StyleSheetSpecification::GetShorthandName(ShorthandId id)
{
	return instance->properties.shorthand_map.GetName(id);
}

std::vector<PropertyId> StyleSheetSpecification::GetShorthandUnderlyingProperties(ShorthandId id)
{
	std::vector<PropertyId> result;
	const ShorthandDefinition* shorthand = instance->properties.GetShorthand(id);
	if (!shorthand)
		return result;

	result.reserve(shorthand->items.size());
	for (auto& item : shorthand->items)
	{
		if (item.type == ShorthandItemType::Property)
		{
			result.push_back(item.property_id);
		}
		else if (item.type == ShorthandItemType::Shorthand)
		{
			// When we have a shorthand pointing to another shorthands, call us recursively
			std::vector<PropertyId> new_items = GetShorthandUnderlyingProperties(item.shorthand_id);
			result.insert(result.end(), new_items.begin(), new_items.end());
		}
	}
	return result;
}

// Registers Rocket's default parsers.
void StyleSheetSpecification::RegisterDefaultParsers()
{
	RegisterParser("number", new PropertyParserNumber(Property::NUMBER));
	RegisterParser("length", new PropertyParserNumber(Property::LENGTH, Property::PX));
	RegisterParser("length_percent", new PropertyParserNumber(Property::LENGTH_PERCENT, Property::PX));
	RegisterParser("number_length_percent", new PropertyParserNumber(Property::NUMBER_LENGTH_PERCENT, Property::PX));
	RegisterParser("angle", new PropertyParserNumber(Property::ANGLE, Property::RAD));
	RegisterParser("keyword", new PropertyParserKeyword());
	RegisterParser("string", new PropertyParserString());
	RegisterParser(ANIMATION, new PropertyParserAnimation(PropertyParserAnimation::ANIMATION_PARSER));
	RegisterParser(TRANSITION, new PropertyParserAnimation(PropertyParserAnimation::TRANSITION_PARSER));
	RegisterParser(COLOR, new PropertyParserColour());
	RegisterParser(TRANSFORM, new PropertyParserTransform());
}


// Registers Rocket's default style properties.
void StyleSheetSpecification::RegisterDefaultProperties()
{
	// Style property specifications (ala RCSS).

	RegisterProperty(PropertyId::MarginTop, MARGIN_TOP, "0px", false, true)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(PropertyId::MarginRight, MARGIN_RIGHT, "0px", false, true)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(PropertyId::MarginBottom, MARGIN_BOTTOM, "0px", false, true)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(PropertyId::MarginLeft, MARGIN_LEFT, "0px", false, true)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterShorthand(ShorthandId::Margin, MARGIN, "margin-top, margin-right, margin-bottom, margin-left", ShorthandType::Box);

	RegisterProperty(PropertyId::PaddingTop, PADDING_TOP, "0px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(PropertyId::PaddingRight, PADDING_RIGHT, "0px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(PropertyId::PaddingBottom, PADDING_BOTTOM, "0px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(PropertyId::PaddingLeft, PADDING_LEFT, "0px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterShorthand(ShorthandId::Padding, PADDING, "padding-top, padding-right, padding-bottom, padding-left", ShorthandType::Box);

	RegisterProperty(PropertyId::BorderTopWidth, BORDER_TOP_WIDTH, "0px", false, true).AddParser("length");
	RegisterProperty(PropertyId::BorderRightWidth, BORDER_RIGHT_WIDTH, "0px", false, true).AddParser("length");
	RegisterProperty(PropertyId::BorderBottomWidth, BORDER_BOTTOM_WIDTH, "0px", false, true).AddParser("length");
	RegisterProperty(PropertyId::BorderLeftWidth, BORDER_LEFT_WIDTH, "0px", false, true).AddParser("length");
	RegisterShorthand(ShorthandId::BorderWidth, BORDER_WIDTH, "border-top-width, border-right-width, border-bottom-width, border-left-width", ShorthandType::Box);

	RegisterProperty(PropertyId::BorderTopColor, BORDER_TOP_COLOR, "black", false, false).AddParser(COLOR);
	RegisterProperty(PropertyId::BorderRightColor, BORDER_RIGHT_COLOR, "black", false, false).AddParser(COLOR);
	RegisterProperty(PropertyId::BorderBottomColor, BORDER_BOTTOM_COLOR, "black", false, false).AddParser(COLOR);
	RegisterProperty(PropertyId::BorderLeftColor, BORDER_LEFT_COLOR, "black", false, false).AddParser(COLOR);
	RegisterShorthand(ShorthandId::BorderColor, BORDER_COLOR, "border-top-color, border-right-color, border-bottom-color, border-left-color", ShorthandType::Box);

	RegisterShorthand(ShorthandId::BorderTop, BORDER_TOP, "border-top-width, border-top-color", ShorthandType::FallThrough);
	RegisterShorthand(ShorthandId::BorderRight, BORDER_RIGHT, "border-right-width, border-right-color", ShorthandType::FallThrough);
	RegisterShorthand(ShorthandId::BorderBottom, BORDER_BOTTOM, "border-bottom-width, border-bottom-color", ShorthandType::FallThrough);
	RegisterShorthand(ShorthandId::BorderLeft, BORDER_LEFT, "border-left-width, border-left-color", ShorthandType::FallThrough);
	RegisterShorthand(ShorthandId::Border, BORDER, "border-top, border-right, border-bottom, border-left", ShorthandType::Recursive);

	RegisterProperty(PropertyId::Display, DISPLAY, "inline", false, true).AddParser("keyword", "none, block, inline, inline-block");
	RegisterProperty(PropertyId::Position, POSITION, "static", false, true).AddParser("keyword", "static, relative, absolute, fixed");
	RegisterProperty(PropertyId::Top, TOP, "auto", false, false)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockHeight);
	RegisterProperty(PropertyId::Right, RIGHT, "auto", false, false)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(PropertyId::Bottom, BOTTOM, "auto", false, false)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockHeight);
	RegisterProperty(PropertyId::Left, LEFT, "auto", false, false)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);

	RegisterProperty(PropertyId::Float, FLOAT, "none", false, true).AddParser("keyword", "none, left, right");
	RegisterProperty(PropertyId::Clear, CLEAR, "none", false, true).AddParser("keyword", "none, left, right, both");

	RegisterProperty(PropertyId::ZIndex, Z_INDEX, "auto", false, false)
		.AddParser("keyword", "auto")
		.AddParser("number");

	RegisterProperty(PropertyId::Width, WIDTH, "auto", false, true)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(PropertyId::MinWidth, MIN_WIDTH, "0px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(PropertyId::MaxWidth, MAX_WIDTH, "-1px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);

	RegisterProperty(PropertyId::Height, HEIGHT, "auto", false, true)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockHeight);
	RegisterProperty(PropertyId::MinHeight, MIN_HEIGHT, "0px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockHeight);
	RegisterProperty(PropertyId::MaxHeight, MAX_HEIGHT, "-1px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockHeight);

	RegisterProperty(PropertyId::LineHeight, LINE_HEIGHT, "1.2", true, true).AddParser("number_length_percent").SetRelativeTarget(RelativeTarget::FontSize);
	RegisterProperty(PropertyId::VerticalAlign, VERTICAL_ALIGN, "baseline", false, true)
		.AddParser("keyword", "baseline, middle, sub, super, text-top, text-bottom, top, bottom")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::LineHeight);

	RegisterProperty(PropertyId::OverflowX, OVERFLOW_X, "visible", false, true).AddParser("keyword", "visible, hidden, auto, scroll");
	RegisterProperty(PropertyId::OverflowY, OVERFLOW_Y, "visible", false, true).AddParser("keyword", "visible, hidden, auto, scroll");
	RegisterShorthand(ShorthandId::Overflow, "overflow", "overflow-x, overflow-y", ShorthandType::Replicate);
	RegisterProperty(PropertyId::Clip, CLIP, "auto", true, false).AddParser("keyword", "auto, none").AddParser("number");
	RegisterProperty(PropertyId::Visibility, VISIBILITY, "visible", false, false).AddParser("keyword", "visible, hidden");

	// Need some work on this if we are to include images.
	RegisterProperty(PropertyId::BackgroundColor, BACKGROUND_COLOR, "transparent", false, false).AddParser(COLOR);
	RegisterShorthand(ShorthandId::Background, BACKGROUND, BACKGROUND_COLOR, ShorthandType::FallThrough);

	RegisterProperty(PropertyId::Color, COLOR, "white", true, false).AddParser(COLOR);

	RegisterProperty(PropertyId::ImageColor, IMAGE_COLOR, "white", false, false).AddParser(COLOR);
	RegisterProperty(PropertyId::Opacity, OPACITY, "1", true, false).AddParser("number");

	RegisterProperty(PropertyId::FontFamily, FONT_FAMILY, "", true, true).AddParser("string");
	RegisterProperty(PropertyId::FontCharset, FONT_CHARSET, "U+0020-007E", true, false).AddParser("string");
	RegisterProperty(PropertyId::FontStyle, FONT_STYLE, "normal", true, true).AddParser("keyword", "normal, italic");
	RegisterProperty(PropertyId::FontWeight, FONT_WEIGHT, "normal", true, true).AddParser("keyword", "normal, bold");
	RegisterProperty(PropertyId::FontSize, FONT_SIZE, "12px", true, true).AddParser("length").AddParser("length_percent").SetRelativeTarget(RelativeTarget::ParentFontSize);
	RegisterShorthand(ShorthandId::Font, FONT, "font-style, font-weight, font-size, font-family, font-charset", ShorthandType::FallThrough);

	RegisterProperty(PropertyId::TextAlign, TEXT_ALIGN, LEFT, true, true).AddParser("keyword", "left, right, center, justify");
	RegisterProperty(PropertyId::TextDecoration, TEXT_DECORATION, "none", true, false).AddParser("keyword", "none, underline"/*"none, underline, overline, line-through"*/);
	RegisterProperty(PropertyId::TextTransform, TEXT_TRANSFORM, "none", true, true).AddParser("keyword", "none, capitalize, uppercase, lowercase");
	RegisterProperty(PropertyId::WhiteSpace, WHITE_SPACE, "normal", true, true).AddParser("keyword", "normal, pre, nowrap, pre-wrap, pre-line");

	RegisterProperty(PropertyId::Cursor, CURSOR, "", true, false).AddParser("string");

	// Functional property specifications.
	RegisterProperty(PropertyId::Drag, DRAG, "none", false, false).AddParser("keyword", "none, drag, drag-drop, block, clone");
	RegisterProperty(PropertyId::TabIndex, TAB_INDEX, "none", false, false).AddParser("keyword", "none, auto");
	RegisterProperty(PropertyId::Focus, FOCUS, "auto", true, false).AddParser("keyword", "none, auto");
	RegisterProperty(PropertyId::ScrollbarMargin, SCROLLBAR_MARGIN, "0", false, false).AddParser("length");
	RegisterProperty(PropertyId::PointerEvents, POINTER_EVENTS, "auto", true, false).AddParser("keyword", "none, auto");

	// Perspective and Transform specifications
	RegisterProperty(PropertyId::Perspective, PERSPECTIVE, "none", false, false).AddParser("keyword", "none").AddParser("length");
	RegisterProperty(PropertyId::PerspectiveOriginX, PERSPECTIVE_ORIGIN_X, "50%", false, false).AddParser("keyword", "left, center, right").AddParser("length_percent");
	RegisterProperty(PropertyId::PerspectiveOriginY, PERSPECTIVE_ORIGIN_Y, "50%", false, false).AddParser("keyword", "top, center, bottom").AddParser("length_percent");
	RegisterShorthand(ShorthandId::PerspectiveOrigin, PERSPECTIVE_ORIGIN, "perspective-origin-x, perspective-origin-y", ShorthandType::FallThrough);
	RegisterProperty(PropertyId::Transform, TRANSFORM, "none", false, false).AddParser(TRANSFORM);
	RegisterProperty(PropertyId::TransformOriginX, TRANSFORM_ORIGIN_X, "50%", false, false).AddParser("keyword", "left, center, right").AddParser("length_percent");
	RegisterProperty(PropertyId::TransformOriginY, TRANSFORM_ORIGIN_Y, "50%", false, false).AddParser("keyword", "top, center, bottom").AddParser("length_percent");
	RegisterProperty(PropertyId::TransformOriginZ, TRANSFORM_ORIGIN_Z, "0", false, false).AddParser("length");
	RegisterShorthand(ShorthandId::TransformOrigin, TRANSFORM_ORIGIN, "transform-origin-x, transform-origin-y, transform-origin-z", ShorthandType::FallThrough);

	RegisterProperty(PropertyId::Transition, TRANSITION, "none", false, false).AddParser(TRANSITION);
	RegisterProperty(PropertyId::Animation, ANIMATION, "none", false, false).AddParser(ANIMATION);

	instance->properties.property_map.AssertAllInserted(PropertyId::NumDefinedIds);
	instance->properties.shorthand_map.AssertAllInserted(ShorthandId::NumDefinedIds);
}

}
}
