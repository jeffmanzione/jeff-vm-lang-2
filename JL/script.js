const urlParams = new URLSearchParams(window.location.search);
const query = urlParams.get('q');

const rToken = document.getElementById('r-token').innerHTML;

const httpRequest = new XMLHttpRequest();
httpRequest.open("GET", "/" + query + '?r-token=' + rToken);
httpRequest.send();

httpRequest.onreadystatechange = (e) => {
  document.getElementById("content").innerHTML = httpRequest.responseText;
}