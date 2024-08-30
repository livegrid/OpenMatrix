import { GIF } from 'gif.js';

export async function compressAndConvertToGif(file) {
  if (file.type === 'image/gif') {
    return compressGif(file);
  } else {
    return convertImageToGif(file);
  }
}

async function compressGif(file) {
  const gifReader = new GIF.GifReader(new Uint8Array(await file.arrayBuffer()));
  const frames = [];

  for (let i = 0; i < gifReader.numFrames(); i++) {
    const frameInfo = gifReader.frameInfo(i);
    const imageData = gifReader.getFrame(i);
    frames.push({ data: imageData, delay: frameInfo.delay * 10 });
  }

  return createResizedGif(frames);
}

async function convertImageToGif(file) {
  return new Promise((resolve, reject) => {
    const img = new Image();
    img.onload = () => {
      const canvas = document.createElement('canvas');
      canvas.width = 64;
      canvas.height = 64;
      const ctx = canvas.getContext('2d');
      ctx.drawImage(img, 0, 0, 64, 64);
      
      const frame = ctx.getImageData(0, 0, 64, 64);
      createResizedGif([{ data: frame, delay: 0 }]).then(resolve).catch(reject);
    };
    img.onerror = reject;
    img.src = URL.createObjectURL(file);
  });
}

function createResizedGif(frames) {
  return new Promise((resolve, reject) => {
    const gif = new GIF({
      workers: 2,
      quality: 10,
      width: 64,
      height: 64,
    });

    frames.forEach(frame => {
      gif.addFrame(frame.data, { delay: frame.delay });
    });

    gif.on('finished', blob => {
      resolve(blob);
    });

    gif.render();
  });
}