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
    LoginCover.style.textShadow = "none";
    LoginCover.style.color = "transparent";

    // Login and sign up hyperlinks.
    let LoginHLink = document.getElementById("LoginHyperlink");
    LoginHLink.style.color = "white";
    LoginHLink.style.textShadow = TextShadowDefault;

    let SignUpHLink = document.getElementById("SignUpHyperlink");
    SignUpHLink.style.color = "white";
    SignUpHLink.style.top = "60px";
    SignUpHLink.style.textShadow = TextShadowDefault;
}

function OnTopPanelUnHoverEvent(panel)
{
    OnElementUnHover(panel);

    panel.style.height = "40px";
    panel.style.backgroundColor = "rgb(87, 28, 60)";

    // Login cover.
    let LoginCover = document.getElementById("TopPanelLoginCover");
    LoginCover.style.textShadow = TextShadowDefault;
    LoginCover.style.color = LoginCoverColor;

    // Login and sign up hyperlinks.
    let LoginHLink = document.getElementById("LoginHyperlink");
    LoginHLink.style.color = "transparent";
    LoginHLink.style.textShadow = "none";

    let SignUpHLink = document.getElementById("SignUpHyperlink");
    SignUpHLink.style.color = "transparent";
    SignUpHLink.style.top = "15px";
    SignUpHLink.style.textShadow = "none";
}

// Home button.
function OnHomeButtonClickEvent(button) {
    window.location.replace("login/login.html");
}