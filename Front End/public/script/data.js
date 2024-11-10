// Impor app dari fireconfig.js
import app from './config.js';
import { getDatabase, ref, query, orderByKey, limitToLast, get } from "https://www.gstatic.com/firebasejs/11.0.1/firebase-database.js";

// Inisialisasi Realtime Database
const database = getDatabase(app);

// Fungsi untuk mengambil dan menampilkan data
function loadData(selectId) {
    const tableBody = document.getElementById("datalogger");
    tableBody.innerHTML = ""; // Kosongkan tabel sebelum mengisi data

    // Ambil data terakhir dari database
    const dataRef = query(ref(database, `sensor/${selectId}/data`), orderByKey(), limitToLast(15));
    
    get(dataRef).then((snapshot) => {
        if (snapshot.exists()) {
            const dataArray = [];

            // Simpan data ke dalam array
            snapshot.forEach((childSnapshot) => {
                const data = childSnapshot.val();
                const timeFormatted = new Date(data.timestamp * 1000)
                                        .toLocaleTimeString([], { 
                                            year: 'numeric', 
                                            month: '2-digit', 
                                            day: '2-digit', 
                                            hour: '2-digit', 
                                            minute: '2-digit', 
                                            second: '2-digit', 
                                            hour12: false
                                        });
                dataArray.push({
                    date: timeFormatted,
                    temperature: data.temperature,
                    humidity: data.humidity,
                    pressure: data.pressure,
                    dew: data.dew,
                    volt: data.volt
                });
            });

            // Balik array agar data terbaru di atas
            dataArray.reverse();

            // Tambahkan data ke tabel
            dataArray.forEach(entry => {
                const row = document.createElement("tr");
                row.innerHTML = `
                    <td>${entry.date}</td>
                    <td>${entry.temperature}</td>
                    <td>${entry.humidity}</td>
                    <td>${entry.pressure}</td>
                    <td>${entry.dew}</td>
                    <td>${entry.volt}</td>
                `;
                tableBody.appendChild(row);
            });
        } else {
            console.warn("Tidak ada data yang tersedia.");
        }
    }).catch((error) => {
        console.error("Error fetching data: ", error);
    });
}

// Fungsi untuk menangani perubahan ID stasiun
function handleStationChange() {
    const stationSelector = document.getElementById("stationSelector");
    const selectedStation = stationSelector.value;
    loadWeatherData(selectedStation);
}

// Muat data saat halaman siap
document.addEventListener('DOMContentLoaded', () => {
    // Panggil loadWeatherData pertama kali dengan stasiun default
    loadWeatherData();

    // Tambahkan event listener ke dropdown selector
    document.getElementById("stationSelector").addEventListener("change", handleStationChange);

    // Perbarui data setiap 1 menit (60000 milidetik)
    setInterval(() => {
        const stationSelector = document.getElementById("stationSelector");
        loadData(stationSelector.value || 'id-02');
    }, 1000);
});
