<template>
    <v-app class="h-100">
        <v-navigation-drawer v-model="drawer">
            <v-list :lines="false" nav>
                <v-list-item
                    v-for="(item, i) in items"
                    :key="i"
                    :value="item"
                    active-color="primary"
                    @click="active = i"
                    :to="item.path"
                    nav
                >
                    <template v-slot:prepend>
                        <v-icon :icon="item.icon"></v-icon>
                    </template>

                    <v-list-item-title v-text="item.text"></v-list-item-title>
                </v-list-item>
            </v-list>
        </v-navigation-drawer>

        <v-main style="min-height: 300px">
            <v-toolbar title="Fctl">
                <template #prepend>
                    <v-app-bar-nav-icon @click="drawer = !drawer"></v-app-bar-nav-icon>
                </template>
                <template #append>
                    <v-btn
                        :icon="isDark ? mdiWeatherNight : mdiWeatherSunny"
                        @click="toggleDark"
                    ></v-btn>
                </template>
            </v-toolbar>
            <router-view></router-view>
        </v-main>
    </v-app>
</template>

<script setup>
import { ref } from 'vue'
import { mdiWeatherSunny, mdiWeatherNight, mdiHome, mdiCog } from '@mdi/js'
import { useDark, useToggle } from '@vueuse/core'
import { useTheme } from 'vuetify/lib/framework.mjs'
const drawer = ref(true)
const isDark = useDark()
const theme = useTheme()
theme.global.name.value = isDark.value ? 'dark' : 'light'
const toggleDark = () => {
    isDark.value = !isDark.value
    theme.global.name.value = isDark.value ? 'dark' : 'light'
}
const items = [
    { text: 'Home', icon: mdiHome, path: '/' },
    { text: 'Settings', icon: mdiCog, path: '/settings' }
]
const active = ref(0)
</script>
<style lang="less"></style>
