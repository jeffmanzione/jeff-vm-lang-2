const urlParams = new URLSearchParams(window.location.search);
const query = urlParams.get('q');

//document.getElementById("content").innerHTML = `Loading file (${query})...`;

const httpRequest = new XMLHttpRequest();
httpRequest.open("GET", "/" + query);
httpRequest.send();

httpRequest.onreadystatechange = (e) => {
  document.getElementById("content").innerHTML = httpRequest.responseText;
}