<script>
    // @ts-nocheck
    import Button from "@/components/Button.svelte";
    import { state, selectImage, deleteImage} from "@/store";
    import TextButton from './TextButton.svelte';

    export let id, name, size;

    let select_loading = false;
    let delete_loading = false;

    $: formattedName = name.split('.').length > 1 ? name.split('.').slice(0, -1).join('.') : name;

    function humanFileSize(bytes, si=true, dp=1) {
        const thresh = si ? 1000 : 1024;

        if (Math.abs(bytes) < thresh) {
            return bytes + ' B';
        }

        const units = si 
            ? ['kB', 'MB', 'GB', 'TB', 'PB', 'EB', 'ZB', 'YB'] 
            : ['KiB', 'MiB', 'GiB', 'TiB', 'PiB', 'EiB', 'ZiB', 'YiB'];
        let u = -1;
        const r = 10**dp;

        do {
            bytes /= thresh;
            ++u;
        } while (Math.round(Math.abs(bytes) * r) / r >= thresh && u < units.length - 1);


        return bytes.toFixed(dp) + ' ' + units[u];
    }
    
    const select = async () => {
      select_loading = true;
      try {
        await selectImage({ name });
      } catch (err) {
        console.log(err);
      } finally {
        select_loading = false;
      }
    }

    const deleteImg = async () => {
      delete_loading = true;
      try {
        await deleteImage({ name });
      } catch (err) {
        console.log(err);
      } finally {
        delete_loading = false;
      }
    }

    
</script>

<tr class="odd:bg-white/80 odd:dark:bg-zinc-950/50 even:bg-zinc-50/80 even:dark:bg-zinc-900/50">
    <th scope="row" class="px-6 py-6 font-medium text-zinc-900 whitespace-nowrap dark:text-zinc-200">
      { formattedName }
    </th>
    <td class="px-6 py-6">
      { humanFileSize(size) }
    </td>
    <td class="flex flex-row gap-x-3 px-6 py-6">
      <TextButton
        selected={$state.image.selected === id}
        loading={select_loading}
        absolute={false}
        on:click={select}
        text="Select"
      />
      <TextButton loading={delete_loading} selected={false} absolute={false} on:click={deleteImg} text="Delete"></TextButton>

    </td>
</tr>