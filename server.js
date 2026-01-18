const dgram = require('dgram');
const express = require('express');
const socketIO = require('socket.io');

const hostname = '0.0.0.0';
const port = 3000;

const MATRIX_WIDTH = 192;
const MATRIX_HEIGHT = 64;
const UDP_TARGET_HOST = '192.168.1.12';
const UDP_TARGET_PORT = 4510;
const UDP_CHUNK_ROWS = 2; // Keep payloads < 1500 bytes
const UDP_HEADER_SIZE = 18;
const UDP_VERSION = 1;
const PROCESS_INTERVAL = 1000 / 60; // ~60fps max

const udpSocket = dgram.createSocket('udp4');
udpSocket.unref();

const app = express();
const server = app.listen(port, hostname);
app.use(express.static('public'));
const io = socketIO(server);

let frameCounter = 0;

function sendUdpFrame(buffer) {
    const bytesPerPixel = 3;
    if (buffer.length < MATRIX_WIDTH * bytesPerPixel) {
        console.warn(`Ignoring frame (${buffer.length} bytes) - too small for ${MATRIX_WIDTH}px width`);
        return;
    }

    if (buffer.length % bytesPerPixel !== 0) {
        console.warn('Frame payload not aligned to RGB triplets, truncating extra bytes');
        buffer = buffer.slice(0, buffer.length - (buffer.length % bytesPerPixel));
    }

    const totalPixels = buffer.length / bytesPerPixel;
    let height = Math.floor(totalPixels / MATRIX_WIDTH);

    if (height > MATRIX_HEIGHT) {
        height = MATRIX_HEIGHT;
    }

    if (height <= 0) {
        console.warn('Calculated frame height is 0, skipping frame');
        return;
    }

    const bytesPerRow = MATRIX_WIDTH * bytesPerPixel;
    const totalChunks = Math.ceil(height / UDP_CHUNK_ROWS);
    const frameId = frameCounter++ & 0xffff;

    for (let chunkIndex = 0; chunkIndex < totalChunks; chunkIndex++) {
        const rowStart = chunkIndex * UDP_CHUNK_ROWS;
        const rows = Math.min(UDP_CHUNK_ROWS, height - rowStart);
        if (rows <= 0) {
            break;
        }

        const payloadStart = rowStart * bytesPerRow;
        const payloadLength = rows * bytesPerRow;

        const packet = Buffer.allocUnsafe(UDP_HEADER_SIZE + payloadLength);
        packet[0] = 0x4f; // 'O'
        packet[1] = 0x4d; // 'M'
        packet[2] = UDP_VERSION;
        packet[3] = 0x00; // flags (0 = raw)

        packet.writeUInt16LE(frameId, 4);
        packet.writeUInt16LE(chunkIndex, 6);
        packet.writeUInt16LE(totalChunks, 8);
        packet.writeUInt16LE(0, 10); // x offset
        packet.writeUInt16LE(rowStart, 12); // y offset
        packet.writeUInt16LE(MATRIX_WIDTH, 14);
        packet.writeUInt16LE(rows, 16);

        buffer.copy(packet, UDP_HEADER_SIZE, payloadStart, payloadStart + payloadLength);

        udpSocket.send(packet, UDP_TARGET_PORT, UDP_TARGET_HOST, (err) => {
            if (err) {
                console.error('Failed to send UDP chunk', err);
            }
        });
    }
}

io.sockets.on('connection', (socket) => {
    console.log('new connection:', socket.id);

    let lastProcessTime = 0;

    socket.on('e131Data', (data) => {
        const now = Date.now();
        if (now - lastProcessTime < PROCESS_INTERVAL) {
            return;
        }
        lastProcessTime = now;

        const frameBuffer = Buffer.isBuffer(data) ? data : Buffer.from(data);
        sendUdpFrame(frameBuffer);
    });
});

console.log(`Server running at ${hostname}:${port}, streaming UDP to ${UDP_TARGET_HOST}:${UDP_TARGET_PORT}`);