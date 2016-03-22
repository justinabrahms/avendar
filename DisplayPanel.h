#ifndef DISPLAYPANEL_H
#define DISPLAYPANEL_H

#include <string>
#include <sstream>
#include <vector>
#include <memory>

class DisplayPanel
{
    struct Info
    {
        Info(const std::string & lineIn, size_t realSizeIn);

        std::string line;
        size_t realSize;
    };

    public:
        /// Constants
        static const char NoColor = '.';

        /// Types
        enum LineStyle {Style_None, Style_Default, Style_Collapse};
        typedef std::vector<Info> LineInfo;

        struct Options
        {
            Options(char lineColorIn = NoColor, char textColorIn = NoColor);
            Options(LineStyle lineStyleIn, char lineColorIn = NoColor, char textColorIn = NoColor);

            LineStyle lineStyle;
            char lineColor;
            char textColor;
        };

        class Component
        {
            public:
                virtual ~Component() {}
                virtual LineInfo Render(const Options & options) const = 0;
                virtual std::auto_ptr<Component> Clone() const = 0;

            protected:
                Component() {}
                Component(const Component &) {}
                void operator=(const Component &) {}
        };

        class Box : public Component
        {
            public:
                Box() {}
                explicit Box(const std::string & line);
                Box(const std::string & line0, const std::string & line1);
                virtual LineInfo Render(const Options & options) const;
                virtual std::auto_ptr<Component> Clone() const;
                void AddLine(const std::string & line);
                unsigned int AddText(const std::string & text);
                unsigned int LineCount() const;

            private:
                LineInfo m_lines;
        };

        class Split : public Component
        {
            public:
                virtual ~Split();
                void Add(const Component & component);
                void Add(const Component & component0, const Component & component1);
                void Add(const Component & component0, const Component & component1, const Component & component2);

            protected:
                Split();
                Split(const Options & options);
                Split(const Split & other);

                const std::vector<Component *> & Components() const;
                const Options & SplitOptions(const Options & globalOptions) const;

            private:
                void operator=(const Split &);

                std::vector<Component *> m_components;
                Options * m_localOptions;
        };

        class VerticalSplit : public Split
        {
            public:
                VerticalSplit() {}
                VerticalSplit(const Component & component);
                VerticalSplit(const Component & component0, const Component & component1);
                VerticalSplit(const Component & component0, const Component & component1, const Component & component2);
                VerticalSplit(const Options & options);

                virtual LineInfo Render(const Options & options) const;
                virtual std::auto_ptr<Component> Clone() const;
        };

        class HorizontalSplit : public Split
        {
            public:
                HorizontalSplit() {}
                HorizontalSplit(const Component & component);
                HorizontalSplit(const Component & component0, const Component & component1);
                HorizontalSplit(const Component & component0, const Component & component1, const Component & component2);
                HorizontalSplit(const Options & options);

                virtual LineInfo Render(const Options & options) const;
                virtual std::auto_ptr<Component> Clone() const;
        };

        /// Methods
        static std::string Render(const Component & component, const Options & options = Options(), unsigned int minWidth = 0);

    private:
        static std::string RenderHorizontalLine(const Options & options, size_t maxLineLength);

        struct Colorizer
        {
            explicit Colorizer(std::string & text, char color);
            ~Colorizer();

            private:
                Colorizer(const Colorizer &);
                void operator=(const Colorizer &);

                std::string * m_text;
                char m_color;
        };
};

inline DisplayPanel::Box::Box(const std::string & line) {AddLine(line);}
inline DisplayPanel::Box::Box(const std::string & line0, const std::string & line1) {AddLine(line0); AddLine(line1);}
inline unsigned int DisplayPanel::Box::LineCount() const {return m_lines.size();}
inline DisplayPanel::VerticalSplit::VerticalSplit(const Component & component) {Add(component);}
inline DisplayPanel::VerticalSplit::VerticalSplit(const Component & component0, const Component & component1) {Add(component0, component1);}
inline DisplayPanel::VerticalSplit::VerticalSplit(const Component & component0, const Component & component1, const Component & component2) {Add(component0, component1, component2);}
inline DisplayPanel::HorizontalSplit::HorizontalSplit(const Component & component) {Add(component);}
inline DisplayPanel::HorizontalSplit::HorizontalSplit(const Component & component0, const Component & component1) {Add(component0, component1);}
inline DisplayPanel::HorizontalSplit::HorizontalSplit(const Component & component0, const Component & component1, const Component & component2) {Add(component0, component1, component2);}


#endif
