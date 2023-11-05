// Element hovers.
function OnElementHover(element)
{
    element.setAttribute("IsHovered", "true");
}

function OnElementUnHover(element)
{
    element.setAttribute("IsHovered", "false");
}


// Short top panel elements.
function OnTopPanelAnimationEnd(panel)
{
    if (panel.getAttribute("IsHovered") == "false")
    {
        let LoginText = document.createElement("p");
        LoginText.appendChild(document.createTextNode("L o g i n"));
        LoginText.id = "TopPanelLoginCover";
        document.getElementById("TopPanel").appendChild(LoginText);
    }
    else if (panel.getAttribute("IsHovered") == "true")
    {
        document.getElementById("TopPanelLoginCover").remove();
    }
}