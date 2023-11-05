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
var IsTopPanelHovered = false;

function OnTopPanelAnimationEnd(panel)
{
    // Fix for shitty probelm of event firing non-stop.
    let IsCurrentlyHovered = panel.getAttribute("IsHovered") == "true";
    if (IsCurrentlyHovered ==IsTopPanelHovered)
    {
        return;
    }

    // Set elements.
    if (!IsCurrentlyHovered)
    {
        let LoginText = document.createElement("p");
        LoginText.appendChild(document.createTextNode("L o g i n"));
        LoginText.id = "TopPanelLoginCover";
        document.getElementById("TopPanel").appendChild(LoginText);
    }
    else
    {
        document.getElementById("TopPanelLoginCover").remove();
    }

    IsTopPanelHovered = IsCurrentlyHovered;
}