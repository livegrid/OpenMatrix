
<script>
  import ModePageLayout from "@/components/ModePageLayout.svelte";
  import { onMount } from "svelte";
  import { images, fetchImages, handleFileUpload } from "@/store";
  import { get } from "svelte/store";
  import ImageRow from "@/components/ImageRow.svelte";
  import UploadPopup from "@/components/UploadPopup.svelte";
  $: isUploadDisabled = $images && $images.length >= 10;

  let fileInput;
  let isUploading = false;
  let errorMessage = '';
  let showErrorPopup = false;
  
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

  
  async function handleFileInputChange(event) {
    const file = event.target.files[0];
    if (file) {
      try {
        isUploading = true;
        await handleFileUpload(file);
        isUploading = false;
      } catch (error) {
        console.error("File upload failed:", error);
        isUploading = false;
        errorMessage = error.message || "File upload failed. Please try again.";
        showErrorPopup = true;
      }
    }
  }

  function closeErrorPopup() {
    showErrorPopup = false;
    errorMessage = '';
  }

  function triggerFileInput() {
    if (!isUploadDisabled) {
      fileInput.click();
    }
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
      on:change={handleFileInputChange}
      class="hidden"
      bind:this={fileInput}
      disabled={isUploadDisabled || isUploading}
    />
    <button
      on:click={triggerFileInput}
      class="px-4 py-2 bg-zinc-700 hover:bg-zinc-600 text-zinc-200 rounded-md transition-colors duration-200 ease-in-out focus:outline-none focus:ring-2 focus:ring-zinc-500 focus:ring-opacity-50"
      class:opacity-50={isUploadDisabled || isUploading}
      class:cursor-not-allowed={isUploadDisabled || isUploading}
      disabled={isUploadDisabled || isUploading}
    > 
      {#if isUploading}
        Uploading...
      {:else if isUploadDisabled}
        Max Images Reached (10)
      {:else}
        Upload New Image
      {/if}
    </button>
  </div>

  
  {#if showErrorPopup}
    <UploadPopup message={errorMessage} onClose={closeErrorPopup} />
  {/if}
</ModePageLayout>
