<script setup lang="ts">
defineProps<{
  options: { value: string; label: string }[];
}>();

const model = defineModel<string>({ required: true });
</script>

<template>
  <select
    v-model="model"
    class="tab-select min-w-0 flex-1 basis-0 rounded border border-gray-300 bg-white px-2 py-1 text-[13px] text-gray-700"
  >
    <option v-for="o in options" :key="o.value" :value="o.value">
      {{ o.label }}
    </option>
  </select>
  <div class="tab-buttons flex gap-0.5 rounded-md bg-gray-200/70 p-0.5">
    <button
      v-for="o in options"
      :key="o.value"
      class="cursor-pointer rounded-md px-2.5 py-1 text-[12px] font-medium transition-all"
      :class="
        model === o.value
          ? 'bg-white text-gray-800 shadow-sm'
          : 'bg-transparent text-gray-500 hover:text-gray-700'
      "
      @click="model = o.value"
    >
      {{ o.label }}
    </button>
  </div>
</template>

<style scoped>
.tab-buttons {
  display: none;
}
@media (min-width: 768px) {
  .tab-select {
    display: none;
  }
  .tab-buttons {
    display: flex;
  }
}
</style>
