
<script>
  import TextSizeButton from "@/components/TextSizeButton.svelte";

  let text = {
    value: '',
    size: 0
  }

  let sizes = [
    {
      id: 0,
      name: 'S'
    },
    {
      id: 1,
      name: 'M'
    },
    {
      id: 2,
      name: 'L'
    }
  ];

</script>

<!-- Your content -->
<div>
  <div class="flex flex-wrap items-center gap-6 sm:flex-nowrap sm:justify-between">
    <h1 class="text-base text-lg font-semibold leading-7 text-gray-900 dark:text-zinc-300">Text</h1>
  </div>
</div>

<div class="mt-8">
  <form>
    <div class="w-full mb-4 border border-gray-200 rounded-md bg-white/50 dark:bg-zinc-950/50 dark:border-zinc-900">
      <div class="px-4 py-2 bg-transparent rounded-t-lg">
        <label for="text" class="sr-only">Your text</label>
        <textarea id="text" rows="4" bind:value={text.value} class="w-full px-0 text-sm text-gray-900 bg-transparent border-0 focus:ring-0 dark:text-white dark:placeholder-gray-400" placeholder="Write a message..." required ></textarea>
      </div>
      <div class="flex items-center justify-between px-4 py-3 border-t dark:border-zinc-900">
        <button type="submit" class="inline-flex order-last items-center py-2.5 px-4 text-xs font-medium text-center text-white bg-emerald-700 rounded-md focus:ring-4 focus:ring-emerald-200 dark:focus:ring-emerald-900 hover:bg-emerald-800">
          Save
        </button>
        <div class="flex space-x-1 rtl:space-x-reverse items-center">
          {#each sizes as size }
            <TextSizeButton on:click={() => text.size = size.id} name={size.name} selected={text.size === size.id} />
          {/each}
        </div>
      </div>
    </div>
  </form>
</div>