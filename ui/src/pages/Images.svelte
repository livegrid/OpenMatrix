
<script>
  import ModePageLayout from "@/components/ModePageLayout.svelte";
  import { onMount } from "svelte";
  import { images, fetchImages } from "@/store";
  import { get } from "svelte/store";
  import ImageRow from "@/components/ImageRow.svelte";
  import * as GIFModule from '@dhdbstjr98/gif.js'
  
  const GIF = GIFModule.default || GIFModule;
  let fileInput;
  const GIF_SIZE = 78;

  const fetch = async () => {
    try {
      await fetchImages();
    } catch (err) {
      console.log(err);
    }
  }

  onMount(async () => {
    if (get(images) === null) {
      setTimeout(fetch, 2000);
    } else {
      await fetch();
    }
  })

  
  async function compressAndConvertToGif(file) {
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

        gif.on('progress', (p) => {
          console.log(`GIF encoding progress: ${Math.round(p * 100)}%`);
        });

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

  async function handleFileUpload(event) {
    const file = event.target.files[0];
    if (file) {
        try {
            const processedGif = await compressAndConvertToGif(file);
            
            // Create a new File object with the processed GIF
            const gifFile = new File([processedGif], file.name.replace(/\.[^/.]+$/, ".gif"), {
                type: "image/gif"
            });
            // Create FormData and append the file
            const formData = new FormData();
            formData.append("file", gifFile, gifFile.name);
            
            // Send the file to the server
            console.log("Uploading file:", gifFile.name, "Size:", gifFile.size);
            try {
                const response = await fetch("/upload", {
                    method: "POST",
                    body: formData
                });
                
                if (response && response.ok) {
                    console.log("File uploaded successfully");
                    // Refresh the image list
                    await fetch();
                } else {
                    console.error("File upload failed", response);
                    // TODO: Show error message to user
                }
            } catch (fetchError) {
                console.error("Fetch error:", fetchError);
                // TODO: Show network error message to user
            }
        } catch (error) {
            console.error("Error processing image:", error);
            // TODO: Show error message to user
        }
    }
}

  function triggerFileInput() {
    fileInput.click();
  }

</script>

<!-- ... rest of the component remains the same ... -->

<ModePageLayout name={'Images'} id={2}>
  <!-- list -->
  {#if Array.isArray($images)}
    <div class="relative overflow-x-auto sm:rounded">
      <table class="w-full text-sm text-left rtl:text-right text-zinc-500 dark:text-zinc-400 border border-gray-200 dark:border-zinc-900">
        <thead class="text-xs text-zinc-700 uppercase bg-zinc-50/80 dark:bg-zinc-900/50 dark:text-zinc-300">
          <tr>
            <th scope="col" class="px-6 py-3">
              Name
            </th>
            <th scope="col" class="px-6 py-3">
              Size
            </th>
            <th scope="col" class="px-6 py-3">
              Action
            </th>
          </tr>
        </thead>
        <tbody>
            {#each $images as image (image.name)}
              <ImageRow id={image.name} name={image.name} size={image.size} />
            {/each}
        </tbody>
      </table>
    </div>
  {:else}
    <div role="status" class="animate-pulse">
      <div class="bg-gray-300/50 rounded dark:bg-zinc-700/50 w-full h-[100px]"></div>
    </div>
    <div role="status" class="animate-pulse mt-3">
      <div class="bg-gray-300/50 rounded dark:bg-zinc-700/50 w-full h-[100px]"></div>
    </div>
    <div role="status" class="animate-pulse mt-3">
      <div class="bg-gray-300/50 rounded dark:bg-zinc-700/50 w-full h-[100px]"></div>
    </div>
  {/if}
  <div class="mt-6 flex justify-center">
    <input
      type="file"
      accept="image/*"
      on:change={handleFileUpload}
      class="hidden"
      bind:this={fileInput}
    />
    <button
      on:click={triggerFileInput}
      class="px-4 py-2 bg-zinc-700 hover:bg-zinc-600 text-zinc-200 rounded-md transition-colors duration-200 ease-in-out focus:outline-none focus:ring-2 focus:ring-zinc-500 focus:ring-opacity-50"
    >
      Upload New Image
    </button>
  </div>
</ModePageLayout>
