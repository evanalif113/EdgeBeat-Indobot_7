const texts = [
    "Projek Akhir WEMOS B Indobot 7",
    "Selamat Datang di Edgebeat",
    "Aplikasi Pemantauan Kesehatan"
];

let count = 0;
let index = 0;
let currentText = '';
let letter = '';
let isDeleting = false;

(function type() {
    if (count === texts.length) {
        count = 0;
    }
    currentText = texts[count];

    if (isDeleting) {
        letter = currentText.slice(0, --index);
    } else {
        letter = currentText.slice(0, ++index);
    }

    document.querySelector('#typing-text').textContent = letter;

    let typeSpeed = 150;
    if (isDeleting) {
        typeSpeed /= 2;
    }

    if (!isDeleting && letter.length === currentText.length) {
        typeSpeed = 2000; // Wait for 2 seconds before starting to delete
        isDeleting = true;
    } else if (isDeleting && letter.length === 0) {
        isDeleting = false;
        count++;
        typeSpeed = 500; // Wait for 0.5 seconds before typing the next text
    }

    setTimeout(type, typeSpeed);
}());