<script>
    // @ts-nocheck
    import SelectButton from "@/components/SelectButton.svelte";
    import Button from "@/components/Button.svelte";
    import { state, selectImage, invokePreview } from "@/store";

    export let id, name, size;

    let select_loading = false;
    let preview_loading = false;
    
    const select = async () => {
      select_loading = true;
      try {
        await selectImage({ id });
      } catch (err) {
        console.log(err);
      } finally {
        select_loading = false;
      }
    }

    const preview = async () => {
        preview_loading = true;
        try { 
            await invokePreview({ id });
        } catch (err) {
            console.log(err);
        } finally {
            preview_loading = false;
        }
    }
</script>

<tr class="odd:bg-white/80 odd:dark:bg-zinc-950/50 even:bg-zinc-50/80 even:dark:bg-zinc-900/50">
    <th scope="row" class="px-6 py-6 font-medium text-zinc-900 whitespace-nowrap dark:text-zinc-200">
      {name}
    </th>
    <td class="px-6 py-6">
      {size}
    </td>
    <td class="flex flex-row gap-x-3 px-6 py-6">
      <SelectButton
        selected={$state.image.selected === id}
        loading={select_loading}
        absolute={false}
        on:click={select}
      />
      {#if $state.image.selected !== id}
        <Button
            on:click={preview}
            loading={preview_loading}
        >
            <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="currentColor" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="w-3 h-3"><polygon points="6 3 20 12 6 21 6 3"/></svg>
            Preview
        </Button>
      {/if}
    </td>
</tr>