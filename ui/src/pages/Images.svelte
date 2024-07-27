
<script>
  import ImageCard from "@/components/ImageCard.svelte";

  const images = [
    {
      name: 'Ken Perlin',
      subtitle: 'Simplex Noise',
    },
    {
      name: 'James Fox',
      subtitle: 'Dancer',
    },
    {
      name: 'James Fox',
      subtitle: 'Colorful',
    }
  ];
</script>

<div>
  <div class="flex flex-wrap items-center gap-6 sm:flex-nowrap sm:justify-between">
    <h1 class="text-base text-lg font-semibold leading-7 text-gray-900 dark:text-zinc-300">Images</h1>
  </div>
</div>
<div class="mt-8">
  <!-- <h3 class="text-base font-semibold leading-6 text-gray-900 dark:text-zinc-300">Last 7 days</h3> -->
  <dl class="grid grid-cols-2 gap-5 sm:grid-cols-3 lg:grid-cols-4">
    {#each images as image}
      <ImageCard title={image.name} subtitle={image.subtitle} />
    {/each}
  </dl>
</div>