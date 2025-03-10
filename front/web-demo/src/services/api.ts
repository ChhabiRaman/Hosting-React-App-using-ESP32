export const getSystemInfo = async () => {
    const response = await fetch(`/api/v1/system/info`);
    const data = await response.json();

    return data;
};