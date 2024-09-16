import { get, writable } from "svelte/store";
import { fetchWithTimeout } from "./helpers";
import * as GIFModule from '@dhdbstjr98/gif.js'
  
const GIF = GIFModule.default || GIFModule;
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
    return new Promise((resolve, reject) => {
      const reader = new FileReader();
      reader.onload = function(e) {
        const arrayBuffer = e.target.result;
        
        console.log("Original GIF loaded, size:", arrayBuffer.byteLength);
  
        const gif = new GIF({
          workers: 2,
          quality: 10,
          width: GIF_SIZE,
          height: GIF_SIZE,
          workerScript: '/path/to/gif.worker.js', // Make sure this path is correct
          debug: true // Enable debug logging
        });
  
        const img = new Image();
        img.onload = function() {
          console.log("Image loaded, dimensions:", img.width, "x", img.height);
  
          const canvas = document.createElement('canvas');
          const ctx = canvas.getContext('2d');
          canvas.width = GIF_SIZE;
          canvas.height = GIF_SIZE;
  
          // Draw the entire GIF onto the canvas
          ctx.drawImage(img, 0, 0, img.width, img.height, 0, 0, GIF_SIZE, GIF_SIZE);
          
          console.log("Image resized and drawn to canvas");
  
          // Add the resized frame to the new GIF
          gif.addFrame(ctx, {delay: 200}); // You might want to adjust the delay
  
          console.log("Frame added to new GIF");
  
          gif.on('progress', function(p) {
            console.log("Rendering progress:", Math.round(p * 100) + "%");
          });
  
          gif.on('finished', function(blob) {
            console.log("GIF rendering finished, size:", blob.size);
            resolve(blob);
          });
  
          console.log("Starting GIF render");
          gif.render();
        };
  
        img.onerror = function(error) {
          console.error("Error loading image:", error);
          reject(error);
        };
  
        img.src = URL.createObjectURL(new Blob([arrayBuffer], {type: 'image/gif'}));
      };
      reader.onerror = function(error) {
        console.error("Error reading file:", error);
        reject(error);
      };
      reader.readAsArrayBuffer(file);
    });
  }
  
  async function convertToGif(file) {
    return new Promise((resolve, reject) => {
      const img = new Image();
      img.onload = () => {
        const gif = new GIF({
          workers: 2,
          quality: 10,
          width: GIF_SIZE,
          height: GIF_SIZE
        });
  
        const canvas = document.createElement('canvas');
        const ctx = canvas.getContext('2d');
        canvas.width = GIF_SIZE;
        canvas.height = GIF_SIZE;
        ctx.drawImage(img, 0, 0, GIF_SIZE, GIF_SIZE);
  
        gif.addFrame(ctx, {delay: 500});
  
        gif.on('finished', (blob) => {
          console.log('GIF creation finished');
          resolve(blob);
        });
  
        console.log('Starting GIF rendering');
        gif.render();
      };
      img.onerror = (error) => {
        console.error('Error loading image:', error);
        reject(error);
      };
      img.src = URL.createObjectURL(file);
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
