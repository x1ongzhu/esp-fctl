<template>
    <v-container>
        <v-row>
            <v-col class="pa-2">
                <v-card elevation="2">
                    <template #subtitle>
                        <span class="d-flex align-center">
                            <v-icon :icon="mdiSineWave" class="mr-2"></v-icon>PWM
                        </span>
                    </template>
                    <template #text> <span style="font-size: 24px">25MHz</span> </template>
                </v-card>
            </v-col>
            <v-col class="pa-2">
                <v-card elevation="2">
                    <template #subtitle>
                        <span class="d-flex align-center">
                            <v-icon :icon="mdiSpeedometer" class="mr-2"></v-icon>RPM
                        </span>
                    </template>
                    <template #text> <span style="font-size: 24px">1600</span> </template>
                </v-card>
            </v-col>
        </v-row>
        <v-row>
            <v-col class="pa-2">
                <v-card elevation="2">
                    <template #subtitle>
                        <span class="d-flex align-center">
                            <v-icon :icon="mdiFan" class="mr-2"></v-icon>Speed
                        </span>
                    </template>
                    <template #text>
                        <div class="text-right">
                            <span style="font-size: 24px">{{ speed }}</span
                            ><span>%</span>
                        </div>
                        <v-slider
                            class="speed-slider mt-2"
                            step="1"
                            v-model="speed"
                            color="purple"
                            :model-value="speed"
                            @update:model-value="change"
                        ></v-slider>
                    </template>
                </v-card>
            </v-col>
        </v-row>
    </v-container>
</template>

<script setup>
import { mdiSineWave, mdiSpeedometer, mdiFan } from '@mdi/js'
import { ref } from 'vue'
import axios from 'axios'
import { useDebounceFn } from '@vueuse/core'
const speed = ref(0)
axios.get('http://192.168.50.123/api/fan/speed').then((res) => {
    speed.value = res.data.speed
})
const change = useDebounceFn((e) => {
    speed.value = e
    axios.put('http://192.168.50.123/api/fan/speed', { speed: e })
}, 100)
</script>
<style lang="scss" scoped>
.speed-slider {
    :deep(.v-input__details) {
        display: none;
    }
}
</style>
