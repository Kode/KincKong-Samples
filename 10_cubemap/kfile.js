const project = new Project('CubeMap');

await project.addProject('../Kinc', {kong: true});

project.addFile('Sources/**');
project.addKongDir('Shaders');
project.setDebugDir('Deployment');

project.flatten();

resolve(project);
