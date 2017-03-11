#include "DisplayPanel.h"
#include <iostream>

int main()
{
    DisplayPanel::Box box;
    box.AddLine("Shield");
    box.AddLine("Sword");
    box.AddLine("Staff");
    box.AddLine("Mace");
    box.AddLine("Knife");
    box.AddLine("Haymaker of Doom");

    DisplayPanel::Box box2;
    box2.AddLine("3p");
    box2.AddLine("sold out");
    box2.AddLine("55 gold pieces of 8");
    box2.AddLine("$1");

    DisplayPanel::HorizontalSplit split1;
    split1.AddComponent(box);
    split1.AddComponent(box2);

    DisplayPanel::Box box3;
    box3.AddLine("Inventory for stuff and things");
    box3.AddLine("Feel free to browse!");

    DisplayPanel::VerticalSplit split2;
    split2.AddComponent(box3);
    split2.AddComponent(split1);

    std::cout << DisplayPanel::Render(split2, DisplayPanel::Options('W', 'y')) << std::endl;
    return 0;
}
