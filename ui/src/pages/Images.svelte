
<script>
  import ModePageLayout from "@/components/ModePageLayout.svelte";
  import { onMount } from "svelte";
  import { images, fetchImages } from "@/store";
  import { get } from "svelte/store";
  import ImageRow from "@/components/ImageRow.svelte";

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
</script>

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
</ModePageLayout>
