
<script>
  import ModePageLayout from "@/components/ModePageLayout.svelte";
  import ImageCard from "@/components/ImageCard.svelte";

  const images = [
    {
      name: 'Simplex Noise',
    },
    {
      name: 'Dancer',
    },
    {
      name: 'Colorful',
    }
  ];
</script>

<ModePageLayout name={'Images'} id={2}>
  <!-- <h3 class="text-base font-semibold leading-6 text-gray-900 dark:text-zinc-300">Last 7 days</h3> -->
  <dl class="grid grid-cols-2 gap-5 sm:grid-cols-3 lg:grid-cols-4">
    {#each images as image}
      <ImageCard name={image.name} selected={false} />
    {/each}
  </dl>
</ModePageLayout>
