// Fields.
const TextShadowDefault = "3px 3px 3px rgba(0, 0, 0, 0.466)";
const LoginCoverColor = "rgba(206, 180, 204, 1)";


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
function OnTopPanelHoverEvent(panel)
{
    OnElementHover(panel);

    panel.style.height = "120px";
    panel.style.backgroundColor = "rgb(141, 44, 97)";

    // Login cover.
    let LoginCover = document.getElementById("TopPanelLoginCover");
    if (LoginCover != null)
    {
        LoginCover.style.textShadow = "none";
        LoginCover.style.color = "transparent";
    }

    // Login and sign up hyperlinks.
    let LoginHLink = document.getElementById("LoginHyperlink");
    if (LoginHLink != null)
    {
        LoginHLink.style.color = "white";
        LoginHLink.style.textShadow = TextShadowDefault;
    }

    let SignUpHLink = document.getElementById("SignUpHyperlink");
    if (SignUpHLink != null)
    {
        SignUpHLink.style.color = "white";
        SignUpHLink.style.top = "60px";
        SignUpHLink.style.textShadow = TextShadowDefault;
    }

    // Home button.
    let HomeButton = document.getElementById("BackToIndexButton")
    if (HomeButton != null)
    {
        HomeButton.style.width = "110px";
        HomeButton.style.height = "110px";
    }
}

function OnTopPanelUnHoverEvent(panel)
{
    OnElementUnHover(panel);

    panel.style.height = "40px";
    panel.style.backgroundColor = "rgb(87, 28, 60)";

    // Login cover.
    let LoginCover = document.getElementById("TopPanelLoginCover");
    if (LoginCover != null)
    {
        LoginCover.style.textShadow = TextShadowDefault;
        LoginCover.style.color = LoginCoverColor;
    }

    // Login and sign up hyperlinks.
    let LoginHLink = document.getElementById("LoginHyperlink");
    if (LoginHLink != null)
    {
        LoginHLink.style.color = "transparent";
        LoginHLink.style.textShadow = "none";
    }

    let SignUpHLink = document.getElementById("SignUpHyperlink");
    if (SignUpHLink != null)
    {
        SignUpHLink.style.color = "transparent";
        SignUpHLink.style.top = "15px";
        SignUpHLink.style.textShadow = "none";
    }

    // Home button.
    let HomeButton = document.getElementById("BackToIndexButton")
    if (HomeButton != null)
    {
        HomeButton.style.width = "30px";
        HomeButton.style.height = "30px";
    }
}