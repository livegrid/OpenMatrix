import { get, writable } from "svelte/store";
import { fetchWithTimeout } from "./helpers";
import { encode, decode, decodeFrames } from 'modern-gif';
import workerUrl from 'modern-gif/worker?url';

// const GIF = GIFModule.default || GIFModule;
const GIF_SIZE = 78;

const API_URL = `.`;

// Status
export const connected = writable(false);
export const theme = writable("dark");

// State
export const state = writable({});

export const images = writable(null);

const fetchState = async () => {
  try {
    const response = await fetchWithTimeout(`${API_URL}/openmatrix/state`, {
      method: "GET",
      timeout: 2000,
    });

    if (
      response.status === 200 &&
      response.headers.get("Content-Type") === "application/json"
    ) {
      const json = await response.json();
      state.set(json);
    } else {
      state.set({});
    }
  } catch (err) {
    console.error(err);
  }
};

export const fetchImages = async () => {
  try {
    const response = await fetchWithTimeout(`${API_URL}/openmatrix/image`, {
      method: "GET",
      timeout: 2000,
    });

    if (
      response.status === 200 &&
      response.headers.get("Content-Type") === "application/json"
    ) {
      const json = await response.json();
      if (Array.isArray(json)) {
        images.set(json);
      }
    } else {
      images.set([]);
    }
  } catch (err) {
    console.error(err);
  }
};

setTimeout(() => {
  fetchState();
  setInterval(fetchState, 2000);
}, 1000);

export const togglePower = async ({ detail }) => {
  const response = await fetchWithTimeout(`${API_URL}./openmatrix/power`, {
    method: "POST",
    timeout: 2000,
    body: JSON.stringify({
      power: detail,
    }),
  });
  if (response.status === 200) {
    state.set({
      ...get(state),
      // @ts-ignore
      power: detail,
    });
  }
  return response;
};

export const toggleAutoBrightness = async ({ detail }) => {
  const response = await fetchWithTimeout(
    `${API_URL}./openmatrix/autobrightness`,
    {
      method: "POST",
      timeout: 2000,
      body: JSON.stringify({
        autobrightness: detail,
      }),
    }
  );
  if (response.status === 200) {
    state.set({
      ...get(state),
      // @ts-ignore
      autobrightness: detail,
    });
  }
  return response;
};

export const updateBrightness = async (value) => {
  const response = await fetchWithTimeout(`${API_URL}/openmatrix/brightness`, {
    method: "POST",
    timeout: 2000,
    body: JSON.stringify({
      brightness: value,
    }),
  });
  if (response.status === 200) {
    state.set({
      ...get(state),
      // @ts-ignore
      brightness: value,
    });
  }
  return response;
};

export const selectMode = async (modeId) => {
  const response = await fetchWithTimeout(`${API_URL}/openmatrix/mode`, {
    method: "POST",
    timeout: 2000,
    body: JSON.stringify({
      mode: modeId,
    }),
  });
  if (response.status === 200) {
    state.set({
      ...get(state),
      // @ts-ignore
      mode: modeId,
    });
  }
  return response;
};

export const selectEffect = async (effect) => {
  const response = await fetchWithTimeout(`${API_URL}/openmatrix/effect`, {
    method: "POST",
    timeout: 2000,
    body: JSON.stringify({
      effect,
    }),
  });
  if (response.status === 200) {
    state.set({
      ...get(state),
      // @ts-ignore
      effects: {
        // @ts-ignore
        ...get(state)?.effects,
        selected: effect,
      },
    });
  }
  return response;
};

export const updateEffectSettings = async (effectId, settings) => {
  const response = await fetchWithTimeout(
    `${API_URL}/openmatrix/effect/settings`,
    {
      method: "POST",
      timeout: 2000,
      body: JSON.stringify({
        effectId,
        settings,
      }),
    }
  );
  if (response.status === 200) {
    state.update((s) => ({
      ...s,
      effects: {
        ...s.effects,
        [effectId]: {
          ...s.effects[effectId],
          ...settings,
        },
      },
    }));
  }
  return response;
};

export const selectImage = async ({ name }) => {
  const response = await fetchWithTimeout(`${API_URL}/openmatrix/image`, {
    method: "POST",
    timeout: 2000,
    body: JSON.stringify({
      name,
    }),
  });
  if (response.status === 200) {
    state.set({
      ...get(state),
      // @ts-ignore
      image: {
        selected: name,
      },
    });
  }
  return response;
};


export const deleteImage = async ({ name }) => {
  const response = await fetchWithTimeout(`${API_URL}/openmatrix/image/delete`, {
    method: "POST",
    timeout: 2000,
    body: JSON.stringify({
      name,
    }),
  });
  if (response.status === 200) {
    state.set({
      ...get(state),
      // @ts-ignore
      image: {
        selected: name,
      },
    });
    // Refresh the image list after successful deletion
    await fetchImages();
  }
  return response;
};

export const handleFileUpload = async (file) => {
  if (file) {
    try {
      const processedImage = await processImage(file);
      
      // Create a new File object with the processed GIF
      const newFileName = file.name.endsWith('.gif') ? file.name : file.name.replace(/\.[^/.]+$/, ".gif");
      const newFile = new File([processedImage], newFileName, {
        type: 'image/gif'
      });
      
      // Create FormData and append the file
      const formData = new FormData();
      formData.append("file", newFile, newFile.name);
      
      // Send the file to the server
      console.log("Uploading file:", newFile.name, "Size:", newFile.size);
      const response = await fetchWithTimeout(`${API_URL}/openmatrix/imageupload`, {
        method: "POST",
        body: formData,
        timeout: 5000, // Increased timeout for file upload
      });
      
      if (response.ok) {
        console.log("File uploaded successfully");
        // Refresh the image list
        await fetchImages();
      } else {
        console.error("File upload failed", response);
        throw new Error("File upload failed");
      }
    } catch (error) {
      console.error("Error processing or uploading image:", error);
      throw error;
    }
  }
};

  async function processImage(file) {
    if (file.type === 'image/gif') {
      return resizeGif(file);
    } else {
      return convertToGif(file);
    }
  }
  
  async function resizeGif(file) {
    return new Promise(async (resolve, reject) => {
      try {
        console.log("Starting GIF resize process");
  
        const arrayBuffer = await file.arrayBuffer();
        console.log("Original GIF loaded, size:", arrayBuffer.byteLength);
  
        const gif = decode(arrayBuffer);
        console.log(`GIF dimensions: ${gif.width}x${gif.height}`);
  
        const frames = await decodeFrames(arrayBuffer, { workerUrl });
        console.log(`Extracted ${frames.length} frames from the GIF`);
  
        const resizedFrames = frames.map(frame => {
          const canvas = document.createElement('canvas');
          const ctx = canvas.getContext('2d');
          canvas.width = GIF_SIZE;
          canvas.height = GIF_SIZE;
  
          // Create a temporary canvas to hold the original frame
          const tempCanvas = document.createElement('canvas');
          const tempCtx = tempCanvas.getContext('2d');
          tempCanvas.width = frame.width;
          tempCanvas.height = frame.height;
          tempCtx.putImageData(new ImageData(frame.data, frame.width, frame.height), 0, 0);
  
          // Draw the frame onto the resized canvas
          ctx.drawImage(tempCanvas, 0, 0, frame.width, frame.height, 0, 0, GIF_SIZE, GIF_SIZE);
  
          // Get the resized image data
          const resizedImageData = ctx.getImageData(0, 0, GIF_SIZE, GIF_SIZE);
  
          return {
            data: resizedImageData.data,
            width: GIF_SIZE,
            height: GIF_SIZE,
            delay: frame.delay,
          };
        });
  
        const output = await encode({
          workerUrl,
          width: GIF_SIZE,
          height: GIF_SIZE,
          frames: resizedFrames,
          maxColors: 256, // You can adjust this for compression
        });
  
        console.log("GIF encoding finished, size:", output.byteLength);
        resolve(new Blob([output], { type: 'image/gif' }));
      } catch (error) {
        console.error("Error processing GIF:", error);
        reject(error);
      }
    });
  }

async function convertToGif(file) {
  return new Promise(async (resolve, reject) => {
    try {
      console.log("Starting image to GIF conversion");

      const img = await createImageBitmap(file);
      console.log(`Original image dimensions: ${img.width}x${img.height}`);

      const canvas = document.createElement('canvas');
      const ctx = canvas.getContext('2d');
      canvas.width = GIF_SIZE;
      canvas.height = GIF_SIZE;

      // Draw and resize the image
      ctx.drawImage(img, 0, 0, img.width, img.height, 0, 0, GIF_SIZE, GIF_SIZE);

      const imageData = ctx.getImageData(0, 0, GIF_SIZE, GIF_SIZE);

      const output = await encode({
        workerUrl,
        width: GIF_SIZE,
        height: GIF_SIZE,
        frames: [{
          data: imageData.data,
          width: GIF_SIZE,
          height: GIF_SIZE,
          delay: 100, // 100ms delay for static image
        }],
        maxColors: 256, // You can adjust this for compression
      });

      console.log("GIF encoding finished, size:", output.byteLength);
      resolve(new Blob([output], { type: 'image/gif' }));
    } catch (error) {
      console.error("Error converting image to GIF:", error);
      reject(error);
    }
  });
}

export const updateText = async ({ payload, size }) => {
  const response = await fetchWithTimeout(`${API_URL}/openmatrix/text`, {
    method: "POST",
    timeout: 2000,
    body: JSON.stringify({
      payload,
      size,
    }),
  });
  if (response.status === 200) {
    state.set({
      ...get(state),
      // @ts-ignore
      text: {
        payload,
        size,
      },
    });
  }
  return response;
};

export const updateMQTTSettings = async (settings) => {
  const response = await fetchWithTimeout(
    `${API_URL}/openmatrix/settings/mqtt`,
    {
      method: "POST",
      timeout: 2000,
      body: JSON.stringify(settings),
    }
  );
  if (response.status === 200) {
    state.set({
      ...get(state),
      settings: {
        ...get(state)?.settings,
        mqtt: settings,
      },
    });
  }
  return response;
};

export const updateDMXSettings = async (settings) => {
  const response = await fetchWithTimeout(
    `${API_URL}/openmatrix/settings/edmx`,
    {
      method: "POST",
      timeout: 2000,
      body: JSON.stringify(settings),
    }
  );
  if (response.status === 200) {
    state.set({
      ...get(state),
      settings: {
        ...get(state)?.settings,
        edmx: settings,
      },
    });
  }
  return response;
};

export const updateHASSSettings = async (settings) => {
  const response = await fetchWithTimeout(
    `${API_URL}/openmatrix/settings/hass`,
    {
      method: "POST",
      timeout: 2000,
      body: JSON.stringify(settings),
    }
  );
  if (response.status === 200) {
    state.set({
      ...get(state),
      settings: {
        ...get(state)?.settings,
        hass: settings,
      },
    });
  }
  return response;
};

export const resetNetwork = async () => {
  const response = await fetchWithTimeout(
    `${API_URL}/openmatrix/settings/network/reset`,
    {
      method: "POST",
      timeout: 2000,
    }
  );
  return response;
};

export const resetFactory = async () => {
  const response = await fetchWithTimeout(
    `${API_URL}/openmatrix/settings/factory/reset`,
    {
      method: "POST",
      timeout: 2000,
    }
  );
  return response;
};
