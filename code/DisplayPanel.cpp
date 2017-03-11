#include "DisplayPanel.h"
#include "StringUtil.h"
#include <algorithm>

/// Info ///
DisplayPanel::Info::Info(const std::string & lineIn, size_t realSizeIn) : line(lineIn), realSize(realSizeIn) {}

/// Options ///
DisplayPanel::Options::Options(char lineColorIn, char textColorIn) : 
    lineStyle(Style_Default), lineColor(lineColorIn), textColor(textColorIn)
{}

DisplayPanel::Options::Options(LineStyle lineStyleIn, char lineColorIn, char textColorIn) : 
    lineStyle(lineStyleIn), lineColor(lineColorIn), textColor(textColorIn)
{}

/// Box ///
DisplayPanel::LineInfo DisplayPanel::Box::Render(const Options & options) const 
{
    LineInfo result;
    for (size_t i(0); i < m_lines.size(); ++i)
    {
        std::string line;
        {
            Colorizer colorizer(line, options.textColor);
            line += m_lines[i].line;
        }
        result.push_back(Info(line, m_lines[i].realSize));
    }
    
    return result;
}

std::auto_ptr<DisplayPanel::Component> DisplayPanel::Box::Clone() const {return std::auto_ptr<Component>(new Box(*this));}
void DisplayPanel::Box::AddLine(const std::string & line)
{
    // Determine the real size of the line
    size_t realSize(0);
    for (size_t i(0); i < line.size(); ++i)
    {
        if (line[i] == '{')
        {
            ++i;
            continue;
        }
        ++realSize;
    }

    m_lines.push_back(Info(line, realSize));
}

unsigned int DisplayPanel::Box::AddText(const std::string & text)
{
    // Split the text into lines
    std::vector<std::string> lines(split_into_lines(text.c_str(), false));
    for (size_t i(0); i < lines.size(); ++i)
        AddLine(lines[i]);

    return lines.size();
}

/// Split ///
DisplayPanel::Split::Split() : m_localOptions(0) {}
DisplayPanel::Split::Split(const Options & localOptions) : m_localOptions(new Options(localOptions)) {}

DisplayPanel::Split::Split(const Split & other) :
    m_localOptions(0)
{
    // Copy the local options
    if (other.m_localOptions != 0)
        m_localOptions = new Options(*other.m_localOptions);

    // Copy the components
    for (size_t i(0); i < other.m_components.size(); ++i)
    {
        std::auto_ptr<Component> otherComponent(other.m_components[i]->Clone());
        m_components.push_back(otherComponent.get());
        otherComponent.release();
    }
}

DisplayPanel::Split::~Split()
{
    for (size_t i(0); i < m_components.size(); ++i)
        delete m_components[i];

    delete m_localOptions;
}

void DisplayPanel::Split::Add(const Component & component) 
{
    std::auto_ptr<Component> copyComponent(component.Clone());
    m_components.push_back(copyComponent.get());
    copyComponent.release();
}

void DisplayPanel::Split::Add(const Component & component0, const Component & component1)
{
    Add(component0);
    Add(component1);
}

void DisplayPanel::Split::Add(const Component & component0, const Component & component1, const Component & component2)
{
    Add(component0);
    Add(component1);
    Add(component2);
}

inline const std::vector<DisplayPanel::Component *> & DisplayPanel::Split::Components() const {return m_components;}

const DisplayPanel::Options & DisplayPanel::Split::SplitOptions(const Options & globalOptions) const 
{
    return (m_localOptions == 0 ? globalOptions : *m_localOptions);
}

/// VerticalSplit ///
DisplayPanel::VerticalSplit::VerticalSplit(const Options & options) : Split(options) {}
std::auto_ptr<DisplayPanel::Component> DisplayPanel::VerticalSplit::Clone() const 
{
    return std::auto_ptr<Component>(new VerticalSplit(*this));
}

DisplayPanel::LineInfo DisplayPanel::VerticalSplit::Render(const Options & globalOptions) const
{
    const Options & options(SplitOptions(globalOptions));

    // Get all the LineInfos, determining the longest line length
    size_t maxLineLength(0);
    std::vector<LineInfo> infos;
    for (size_t i(0); i < Components().size(); ++i)
    {
        infos.push_back(Components()[i]->Render(globalOptions));
        for (size_t j(0); j < infos[i].size(); ++j)
            maxLineLength = std::max(maxLineLength, infos[i][j].realSize);
    }

    // Now consolidate the LineInfos
    LineInfo result;
    for (size_t i(0); i < infos.size(); ++i)
    {
        // Handle dividers and options
        if (i != 0)
        {
            switch (options.lineStyle)
            {
                case Style_None: result.push_back(Info(std::string(), 0)); break;
                case Style_Default: result.push_back(Info(std::string(maxLineLength, '-'), maxLineLength)); break;
                case Style_Collapse: break;
            }
        }

        // Generate the lines for this component
        for (size_t j(0); j < infos[i].size(); ++j)
            result.push_back(infos[i][j]);
    }
    return result;
}

/// HorizontalSplit ///
DisplayPanel::HorizontalSplit::HorizontalSplit(const Options & options) : Split(options) {}
std::auto_ptr<DisplayPanel::Component> DisplayPanel::HorizontalSplit::Clone() const 
{
    return std::auto_ptr<Component>(new HorizontalSplit(*this));
}

DisplayPanel::LineInfo DisplayPanel::HorizontalSplit::Render(const Options & globalOptions) const
{
    const Options & options(SplitOptions(globalOptions));

    // Get all the LineInfos, determining the longest line count
    size_t maxLineCount(0);
    std::vector<LineInfo> infos;
    for (size_t i(0); i < Components().size(); ++i)
    {
        infos.push_back(Components()[i]->Render(globalOptions));
        maxLineCount = std::max(maxLineCount, infos[infos.size() - 1].size());
    }

    // Prepare the result by ensuring sufficient lines
    LineInfo result;
    for (size_t i(0); i < maxLineCount; ++i)
        result.push_back(Info(std::string(), 0));

    // Now consolidate the LineInfos
    size_t maxLineLength(0);
    for (size_t i(0); i < infos.size(); ++i)
    {
        size_t nextMaxLineLength(maxLineLength);

        // Generate the lines for this component
        for (size_t j(0); j < maxLineCount; ++j)
        {
            // Verify sufficiently-padded lines
            if (result[j].realSize < maxLineLength)
            {
                result[j].line.append(maxLineLength - result[j].realSize, ' ');
                result[j].realSize = maxLineLength;
            }

            // Work in divider and options
            if (i != 0)
            {
                switch (options.lineStyle)
                {
                    case Style_None: 
                        result[j].line += ' ';
                        ++result[j].realSize;
                        break;

                    case Style_Collapse: break;
                    case Style_Default: 
                    {
                        Colorizer colorizer(result[j].line, options.lineColor);
                        result[j].line += " | ";
                        result[j].realSize += 3;
                        break;
                    }
                }
            }

            // Add in the next line, if it exists
            if (j < infos[i].size())
            {
                result[j].line += infos[i][j].line;
                result[j].realSize += infos[i][j].realSize;
                nextMaxLineLength = std::max(nextMaxLineLength, result[j].realSize);
            }
        }

        maxLineLength = nextMaxLineLength;
    }
    return result;
}

/// DisplayPanel ///
std::string DisplayPanel::Render(const Component & component, const Options & options, unsigned int minWidth)
{
    std::ostringstream result;
    LineInfo lines(component.Render(options));
   
    // Determine max line length
    size_t maxLineLength(minWidth);
    for (size_t i(0); i < lines.size(); ++i)
        maxLineLength = std::max(maxLineLength, lines[i].realSize);
   
    // Write any header line
    result << RenderHorizontalLine(options, maxLineLength);

    // Write out the lines 
    for (size_t i(0); i < lines.size(); ++i)
    {
        std::string line;
        switch (options.lineStyle)
        {
            case Style_None:        // Fall-through
            case Style_Collapse:    line = lines[i].line; break;
            case Style_Default:
            {
                {
                    Colorizer colorizer(line, options.lineColor);
                    line += "| ";
                }
                line += lines[i].line;
                line.append(maxLineLength - lines[i].realSize, ' ');
                {
                    Colorizer colorizer(line, options.lineColor);
                    line += " |";
                }
                break;
            }
        }

        result << line << '\n';
    }

    // Write any trailer line
    result << RenderHorizontalLine(options, maxLineLength);
    return result.str();
}

std::string DisplayPanel::RenderHorizontalLine(const Options & options, size_t maxLineLength)
{
    std::string line;
    switch (options.lineStyle)
    {
        case Style_None: break;
        case Style_Collapse: break;
        case Style_Default: 
        {
            Colorizer colorizer(line, options.lineColor);
            line += "+-";
            line.append(maxLineLength, '-');
            line += "-+";
        }
        line += '\n';
        break;
    }
    return line;
}

/// Colorizer ///
DisplayPanel::Colorizer::Colorizer(std::string & text, char color) : 
    m_text(&text), m_color(color)
{
    if (m_color != NoColor)
    {
        (*m_text) += '{';
        (*m_text) += m_color;
    }
}

DisplayPanel::Colorizer::~Colorizer()
{
    if (m_color != NoColor)
        (*m_text) += "{x";
}
